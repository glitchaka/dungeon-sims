extends RefCounted
## Preserves the author's Tiled layers, atlas regions, object pivots and collision polygons.
var floor_draws:Array=[]
var objects:Array[Dictionary]=[]
var textures:Dictionary={}
var nav:=AStarGrid2D.new()
var houses:Dictionary={}
var people:Dictionary={}
var time:=0.0

func _init()->void:
	var data:Dictionary=JSON.parse_string(FileAccess.get_file_as_string("res://data/town.json"))
	nav.region=Rect2i(0,0,40,40);nav.cell_size=Vector2(16,16);nav.offset=Vector2(8,8)
	nav.diagonal_mode=AStarGrid2D.DIAGONAL_MODE_NEVER;nav.update()
	for layer in data.layers:
		if not layer.visible:continue
		for i in layer.cells.size():
			var gid:int=int(layer.cells[i]);var tile_id:=gid&0x1fffffff
			if tile_id==0:continue
			var tile:Dictionary=data.tiles[str(tile_id)]
			var p:=Vector2i(i%40,i/40)
			var item:=command(tile,Vector2(p*16),Vector2(16,16),gid)
			item.alpha=layer.get("alpha",1.0);floor_draws.append(item)
			if layer.name=="Water":nav.set_point_solid(p,true)
	for object in data.objects:
		var gid:int=int(object.gid);var tile:Dictionary=data.tiles[str(gid&0x1fffffff)]
		var p:=Vector2(object.pos[0],object.pos[1]);var size_px:=Vector2(object.size[0],object.size[1])
		var item:=command(tile,p,size_px,gid)
		item.id=int(object.id);item.bottom=p.y+size_px.y
		objects.append(item)
		if object.id in [1.0,2.0,3.0,4.0]:houses[int(object.id)]=Rect2(p,size_px)
		for polygon in tile.polygons:
			var poly:=PackedVector2Array()
			for point in polygon:poly.append(p+Vector2(point[0],point[1]))
			for y in range(maxi(0,int(p.y/16)),mini(40,ceili((p.y+size_px.y)/16))):
				for x in range(maxi(0,int(p.x/16)),mini(40,ceili((p.x+size_px.x)/16))):
					if Geometry2D.is_point_in_polygon(Vector2(x*16+8,y*16+8),poly):nav.set_point_solid(Vector2i(x,y),true)
	objects.sort_custom(func(a:Dictionary,b:Dictionary)->bool:return a.bottom<b.bottom)
	for shadow in data.get("shadows",[]):
		var gid:int=int(shadow.gid);var tile:Dictionary=data.tiles[str(gid&0x1fffffff)]
		var item:=command(tile,Vector2(shadow.pos[0],shadow.pos[1]),Vector2(shadow.size[0],shadow.size[1]),gid)
		item.alpha=0.25;floor_draws.append(item)

func command(tile:Dictionary,p:Vector2,size_px:Vector2,gid:int)->Dictionary:
	if not textures.has(tile.path):textures[tile.path]=load(tile.path)
	return {"texture":textures[tile.path],"pos":p,"size":size_px,"source":Rect2(tile.rect[0],tile.rect[1],tile.rect[2],tile.rect[3]),"flip_x":(gid&0x80000000)!=0,"flip_y":(gid&0x40000000)!=0}
func draw_item(canvas:Node2D,item:Dictionary)->void:
	var rect:=Rect2(item.pos,item.size)
	if item.flip_x:rect.size.x=-rect.size.x
	if item.flip_y:rect.size.y=-rect.size.y
	canvas.draw_texture_rect_region(item.texture,rect,item.source,Color(1,1,1,item.get("alpha",1.0)))
func floor_to(canvas:Node2D)->void:
	for item in floor_draws:draw_item(canvas,item)
func nearby(p:Vector2)->Vector2i:
	var origin:=Vector2i(clampi(int(p.x/16),0,39),clampi(int(p.y/16),0,39))
	for r in range(0,20):
		for y in range(maxi(0,origin.y-r),mini(40,origin.y+r+1)):
			for x in range(maxi(0,origin.x-r),mini(40,origin.x+r+1)):
				if not nav.is_point_solid(Vector2i(x,y)):return Vector2i(x,y)
	return Vector2i(20,15)
func door(index:int)->Vector2:
	var house:Rect2=houses[index]
	var at:=nearby(Vector2(house.get_center().x,house.end.y+12))
	return Vector2(at*16)+Vector2(8,8)
func update(sim,town:int,delta:float)->Array:
	var result:Array=[]
	var center:Vector2=sim.world.center(sim.world.settlements[town].cell)
	var residents:Array=sim.agents.filter(func(a:Dictionary)->bool:return a.home_town==town)
	var slots:=[4,1,2]
	for a in sim.agents:
		if a.location!="world" or a.pos.distance_to(center)>78:continue
		if a.action=="sleep" and a.phase=="act":continue
		var home:=door(slots[maxi(0,residents.find(a))%3])
		var target:=Vector2(297,249)
		match a.action:
			"sleep","eat","drink_home":target=home
			"work","sell_loot":target=door(3)
			"deposit","fetch","tend":target=Vector2(175,380)
			"drink","bathe":target=Vector2(368,480)
			"hunt","gather","explore","expedition","project","sow","harvest","till":
				var direction:Vector2=sim.world.center(a.goal)-center
				target=Vector2(624,260) if direction.x>0 else Vector2(25,220)
				if absf(direction.y)>absf(direction.x):target=Vector2(320,625) if direction.y>0 else Vector2(320,20)
		var destination:=nearby(target)
		var id:="%d:%d"%[town,a.id]
		if not people.has(id):people[id]={"pos":home,"goal":Vector2i(-1,-1),"path":PackedVector2Array(),"index":0}
		var actor:Dictionary=people[id]
		if actor.goal!=destination:
			actor.goal=destination;actor.path=nav.get_point_path(nearby(actor.pos),destination);actor.index=0
		var budget:=delta*35
		while actor.index<actor.path.size() and budget>0:
			var point:Vector2=actor.path[actor.index]
			var distance:float=actor.pos.distance_to(point)
			if distance<=budget:actor.pos=point;budget-=distance;actor.index+=1
			else:actor.pos=actor.pos.move_toward(point,budget);budget=0
		result.append({"agent":a,"pos":actor.pos,"bottom":actor.pos.y})
	return result

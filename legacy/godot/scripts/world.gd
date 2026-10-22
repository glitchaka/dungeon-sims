extends Node2D
## Hand-composed island. All atlas coordinates address original 8 x 8 cells.
const W := 128
const H := 96
const TILE := 8
const SEA := 0
const LAND := 1
const FRESH := 2
const ROAD := 3
const BRIDGE := 4
const HOME := Vector2i(36, 52)
const GRANARY := Vector2i(35,54)
const MARKET := Vector2i(39,52)
const REST_SPOTS := [Vector2i(34,51),Vector2i(37,49),Vector2i(39,55),Vector2i(87,69),Vector2i(90,72),Vector2i(81,22)]
const LAKE := Vector2i(70, 37)
const TILES = preload("res://Res/Pico-8/MAS-Tileset-P8.png")
const OBJECTS = preload("res://Res/Pico-8/MAS-Objects-P8.png")
const ROADS = preload("res://Res/Pico-8/MAS-Road-P8.png")
const FENCES = preload("res://Res/Pico-8/MAS-Fence-P8.png")
const FIELDS = preload("res://Res/Pico-8/MAS-Fields-P8.png")
var terrain := PackedInt32Array()
var features: Dictionary = {}
var decorations: Dictionary = {}
var buildings: Array[Dictionary] = []
var forest: Array[Vector2i] = []
var water_spots: Array[Vector2i] = []
var accessible: Array[Vector2i] = []
var initial_fields: Array[Vector2i] = []
var farm_sites: Array[Vector2i] = []
var sand: Dictionary = {}
var settlements: Array[Dictionary] = []
var dungeon_sites: Array[Dictionary] = []
var ports: Array[Dictionary] = []
var pasture: Array[Vector2i] = []
var nav := AStarGrid2D.new()
var rng := RandomNumberGenerator.new()

func cell_index(p: Vector2i) -> int: return p.y * W + p.x
func inside(p: Vector2i) -> bool: return p.x >= 0 and p.y >= 0 and p.x < W and p.y < H
func tile_at(p: Vector2i) -> int: return terrain[cell_index(p)] if inside(p) else SEA
func center(p: Vector2i) -> Vector2: return Vector2(p * 8) + Vector2(4, 4)
func cell(p: Vector2) -> Vector2i: return Vector2i(floori(p.x / 8), floori(p.y / 8))
func ellipse(x: float, y: float, cx: float, cy: float, rx: float, ry: float) -> float:
	return pow((x - cx) / rx, 2) + pow((y - cy) / ry, 2)

func generate() -> void:
	rng.seed=84421
	terrain.resize(W*H)
	settlements=[{"name":"VALLEVERDE","cell":Vector2i(36,51),"radius":9},{"name":"PUERTO BRUMA","cell":Vector2i(89,69),"radius":8},{"name":"PINAR ALTO","cell":Vector2i(81,20),"radius":6}]
	dungeon_sites=[{"name":"CRIPTA DEL ROBLE","cell":Vector2i(43,19),"level":1},{"name":"MINAS DEL ECO","cell":Vector2i(96,31),"level":2},{"name":"RUINAS DE LA MAREA","cell":Vector2i(23,69),"level":3}]
	for y in H:
		for x in W:
			var p:=Vector2i(x,y)
			var waviness:=0.06*sin(x*0.23)+0.06*sin(y*0.31)+0.03*cos((x+y)*0.47)
			var land:=ellipse(x,y,65,48,54,39)<1.0+waviness
			land=land or ellipse(x,y,17,30,9,12)<1.0 or ellipse(x,y,108,68,10,11)<1.0
			# Deep bays and offshore islets give the continent a readable coastline.
			if ellipse(x,y,48,5,13,9)<1 or ellipse(x,y,8,55,14,8)<1 or ellipse(x,y,122,47,15,10)<1: land=false
			terrain[cell_index(p)]=LAND if land else SEA
			if land and (ellipse(x,y,70,37,12,9)<1+0.12*sin(x*0.6) or ellipse(x,y,45,72,8,5)<1 or (x>=77 and abs(y-(42+sin(x*0.13)*4))<1.2)):
				terrain[cell_index(p)]=FRESH
	for settlement in settlements:
		var c:Vector2i=settlement.cell
		for y in range(c.y-5,c.y+6):
			for x in range(c.x-5,c.x+6): terrain[cell_index(Vector2i(x,y))]=LAND
	# Meandering connected routes linking towns, freshwater, ruins and both ports.
	for route in [[HOME,Vector2i(47,52),Vector2i(47,45),Vector2i(57,45),Vector2i(57,28),Vector2i(67,28),Vector2i(67,22),Vector2i(81,22)], [HOME,Vector2i(36,64),Vector2i(58,64),Vector2i(58,68),Vector2i(89,68)], [Vector2i(89,68),Vector2i(96,68),Vector2i(96,31)], [HOME,Vector2i(27,52),Vector2i(27,69),Vector2i(23,69)], [Vector2i(47,45),Vector2i(43,45),Vector2i(43,19)], [Vector2i(81,22),Vector2i(96,22),Vector2i(96,31)], [Vector2i(58,68),Vector2i(58,76),Vector2i(52,76)]]:
		for i in range(1,route.size()): carve_road(route[i-1],route[i])
	# Tree and mountain masks follow smooth, irregular bands rather than rectangles.
	var types:Dictionary={}
	for y in H:
		for x in W:
			var p:=Vector2i(x,y)
			if tile_at(p)!=LAND or coastal(p): continue
			var near_town:=false
			for town in settlements:
				if Vector2(p-town.cell).length()<town.radius: near_town=true
			if near_town: continue
			var ridge:=minf(ellipse(x,y,40,19,13,7),ellipse(x,y,99,25,10,14))
			var grove:=minf(minf(ellipse(x,y,61,17,19,8),ellipse(x,y,25,40,11,13)),minf(ellipse(x,y,102,62,11,17),ellipse(x,y,67,79,16,6)))
			var edge:=0.15*sin(x*0.75)+0.13*cos(y*0.8)
			if ridge<0.86+edge:
				types[p]="mountain"
			elif grove<1.3+edge:
				forest.append(p)
				if grove<0.85+edge and sin(x*0.38)+cos(y*0.31)>-1.6: types[p]="pine" if x<38 else "tree"
	for p in types:
		var kind:String=types[p]
		var l:bool=types.get(p+Vector2i.LEFT,"")==kind
		var r:bool=types.get(p+Vector2i.RIGHT,"")==kind
		var u:bool=types.get(p+Vector2i.UP,"")==kind
		var d:bool=types.get(p+Vector2i.DOWN,"")==kind
		if kind=="mountain": features[p]=Vector2i(1 if not l else (3 if not r else 2),13 if not u else (15 if not d else 14))
		else:
			var sx:=7 if kind=="pine" else 1
			var sy:=7 if kind=="pine" else (4 if p.y<30 else 10)
			features[p]=Vector2i(sx if not l else (sx+2 if not r else sx+1),sy if not u else (sy+2 if not d else sy+1))
	for site in dungeon_sites:
		var c:Vector2i=site.cell
		for y in range(c.y-2,c.y+3):
			for x in range(c.x-2,c.x+3):
				features.erase(Vector2i(x,y)); terrain[cell_index(Vector2i(x,y))]=LAND
	for i in settlements.size():
		var c:Vector2i=settlements[i].cell
		for offset in [Vector2i(-3,-1),Vector2i(0,-3),Vector2i(3,2)]:
			add_building(c+offset,Vector2i(3,0),Vector2i.ONE,"Casa")
		add_building(c+Vector2i(1,-1),Vector2i(6,4),Vector2i(2,2),"Poblado")
		carve_road(c+Vector2i(-5,1),c+Vector2i(5,1))
		carve_road(c+Vector2i(0,-4),c+Vector2i(0,5))
	add_building(Vector2i(34,52),Vector2i(0,2),Vector2i(2,2),"Granero")
	add_building(Vector2i(39,50),Vector2i(4,4),Vector2i(2,2),"Mercado")
	# Coastal docks visibly extend into the sea and are actually used by caravels.
	for px in [31,88]:
		var coast_y:=H-4
		while coast_y>H/2 and tile_at(Vector2i(px,coast_y))==SEA: coast_y-=1
		var c:=Vector2i(px,coast_y-1)
		ports.append({"name":"MUELLE OESTE" if px==31 else "PUERTO BRUMA","cell":c})
		carve_road(Vector2i(px,64 if px==31 else 68),c)
		for y in range(c.y,c.y+5):
			for dx in 2:
				var p:=Vector2i(px+dx,y)
				if inside(p): terrain[cell_index(p)]=BRIDGE; features.erase(p)
	# Only a small starter plot exists. All further cultivation is a living land-use choice.
	for y in range(57,60):
		for x in range(39,45): initial_fields.append(Vector2i(x,y)); features.erase(Vector2i(x,y))
	for y in range(56,61):
		for x in range(29,34):
			var p:=Vector2i(x,y)
			if tile_at(p)==LAND: pasture.append(p); features.erase(p)
	for y in H:
		for x in W:
			var p:=Vector2i(x,y)
			if tile_at(p)!=LAND or features.has(p): continue
			var beach:=false
			if y>67:
				for d in [Vector2i(0,2),Vector2i(0,3),Vector2i(2,0),Vector2i(-2,0)]:
					if tile_at(p+d)==SEA:beach=true
			if beach:sand[p]=true
			elif (x*19+y*7)%67==0: decorations[p]=Vector2i(2+x%4,0)
	for b in buildings:
		for y in b.size.y:
			for x in b.size.x: features.erase(b.cell+Vector2i(x,y))
	for p in REST_SPOTS: features.erase(p); terrain[cell_index(p)]=LAND
	build_navigation()
	queue_redraw()

func reset_world()->void:
	terrain.clear();features.clear();decorations.clear();buildings.clear();forest.clear();water_spots.clear()
	accessible.clear();initial_fields.clear();farm_sites.clear();sand.clear();settlements.clear();dungeon_sites.clear();ports.clear();pasture.clear()
	generate()

func build_navigation() -> void:
	accessible.clear(); water_spots.clear(); farm_sites.clear()
	nav.region=Rect2i(0,0,W,H)
	nav.cell_size=Vector2(8,8)
	nav.offset=Vector2(4,4)
	nav.diagonal_mode=AStarGrid2D.DIAGONAL_MODE_NEVER
	nav.update()
	for y in H:
		for x in W:
			var p:=Vector2i(x,y)
			nav.set_point_solid(p,tile_at(p) in [SEA,FRESH] or features.has(p) or coastal(p))
	for b in buildings:
		for y in b.size.y:
			for x in b.size.x: nav.set_point_solid(b.cell+Vector2i(x,y))
	var seen:Dictionary={HOME:true}
	var pending:Array[Vector2i]=[HOME]
	var cursor:=0
	while cursor<pending.size():
		var p:=pending[cursor]
		cursor+=1
		accessible.append(p)
		for d in [Vector2i.LEFT,Vector2i.RIGHT,Vector2i.UP,Vector2i.DOWN]:
			var q:Vector2i=p+d
			if tile_at(q)==FRESH and not water_spots.has(p): water_spots.append(p)
			if inside(q) and not seen.has(q) and not nav.is_point_solid(q):
				seen[q]=true
				pending.append(q)
	forest=forest.filter(func(p:Vector2i)->bool:return seen.has(p))
	initial_fields=initial_fields.filter(func(p:Vector2i)->bool:return seen.has(p))
	for p in accessible:
		if tile_at(p)==LAND and not sand.has(p) and not pasture.has(p): farm_sites.append(p)

func coastal(p: Vector2i) -> bool:
	if tile_at(p) in [SEA,FRESH,BRIDGE]: return false
	for d in [Vector2i.LEFT,Vector2i.RIGHT,Vector2i.UP,Vector2i.DOWN]:
		if tile_at(p+d)==SEA: return true
	return false

func carve_road(start:Vector2i,finish:Vector2i)->void:
	var p:=start
	var length:=maxi(absi(finish.x-start.x),absi(finish.y-start.y))
	for i in range(length+1):
		var t:=float(i)/maxi(1,length)
		var q:=Vector2i(Vector2(start).lerp(Vector2(finish),t).round())
		if length>12:
			var bend:=roundi(sin(t*PI)*sin(t*PI*1.4+start.x*0.15)*3)
			if absi(finish.x-start.x)>absi(finish.y-start.y):q.y+=bend
			else:q.x+=bend
		while true:
			if inside(p) and tile_at(p)!=SEA:terrain[cell_index(p)]=BRIDGE if tile_at(p)==FRESH else ROAD
			if p==q:break
			if p.x!=q.x:p.x+=signi(q.x-p.x)
			else:p.y+=signi(q.y-p.y)

func add_building(p:Vector2i,source:Vector2i,size_cells:Vector2i,title:String)->void:
	buildings.append({"cell":p,"source":source,"size":size_cells,"title":title})
func walkable(p:Vector2i)->bool: return inside(p) and not nav.is_point_solid(p)
func path(from:Vector2,to:Vector2i)->PackedVector2Array:
	if not walkable(cell(from)) or not walkable(to): return PackedVector2Array()
	return nav.get_point_path(cell(from),to)
func nearest(from:Vector2,options:Array[Vector2i])->Vector2i:
	var best:=HOME
	var distance:=INF
	for p in options:
		var d:=from.distance_squared_to(center(p))
		if d<distance:
			distance=d
			best=p
	return best
func atlas(texture:Texture2D,p:Vector2,source:Vector2i,tint:=Color.WHITE)->void:
	draw_texture_rect_region(texture,Rect2(p,Vector2(8,8)),Rect2(Vector2(source*8),Vector2(8,8)),tint)

func road_source(p:Vector2i)->Vector2i:
	var n:=tile_at(p+Vector2i.UP) in [ROAD,BRIDGE]
	var s:=tile_at(p+Vector2i.DOWN) in [ROAD,BRIDGE]
	var e:=tile_at(p+Vector2i.RIGHT) in [ROAD,BRIDGE]
	var w:=tile_at(p+Vector2i.LEFT) in [ROAD,BRIDGE]
	if n and s and e and w: return Vector2i(2,2)
	if n and s and e: return Vector2i(1,2)
	if n and s and w: return Vector2i(3,2)
	if w and e and s: return Vector2i(2,1)
	if w and e and n: return Vector2i(2,3)
	if e and s: return Vector2i(1,1)
	if w and s: return Vector2i(3,1)
	if e and n: return Vector2i(1,3)
	if w and n: return Vector2i(3,3)
	if n or s: return Vector2i(0,2) if n and s else (Vector2i(0,3) if n else Vector2i(0,1))
	return Vector2i(2,0) if e and w else (Vector2i(1,0) if e else Vector2i(3,0))

func _draw()->void:
	if terrain.is_empty(): return
	for y in H:
		for x in W:
			var p:=Vector2i(x,y)
			var t:=tile_at(p)
			var source:=Vector2i(1,0)
			if t in [SEA,FRESH]: source=Vector2i(0,1)
			else:
				var l:=tile_at(p+Vector2i.LEFT) in [SEA,FRESH]
				var r:=tile_at(p+Vector2i.RIGHT) in [SEA,FRESH]
				var u:=tile_at(p+Vector2i.UP) in [SEA,FRESH]
				var d:=tile_at(p+Vector2i.DOWN) in [SEA,FRESH]
				if l or r or u or d:
					source=Vector2i(6 if l else (8 if r else 7),0 if u else (2 if d else 1))
				elif tile_at(p+Vector2i(1,1)) in [SEA,FRESH]: source=Vector2i(3,1)
				elif tile_at(p+Vector2i(-1,1)) in [SEA,FRESH]: source=Vector2i(5,1)
				elif tile_at(p+Vector2i(1,-1)) in [SEA,FRESH]: source=Vector2i(3,3)
				elif tile_at(p+Vector2i(-1,-1)) in [SEA,FRESH]: source=Vector2i(5,3)
				elif sand.has(p): source=Vector2i(7,5)
				elif decorations.has(p): source=decorations[p]
			atlas(TILES,Vector2(p*8),source)
			if t==ROAD: atlas(ROADS,Vector2(p*8),road_source(p))
			elif t==BRIDGE: atlas(ROADS,Vector2(p*8),Vector2i(0,5))
	for p in features: atlas(TILES,Vector2(p*8),features[p])
	for b in buildings:
		for y in b.size.y:
			for x in b.size.x: atlas(OBJECTS,Vector2((b.cell+Vector2i(x,y))*8),b.source+Vector2i(x,y))
func settlement_at(p:Vector2i)->int:
	for i in settlements.size():
		if Vector2(p-settlements[i].cell).length()<5: return i
	return -1
func set_road(p:Vector2i)->void:
	terrain[cell_index(p)]=ROAD; features.erase(p); build_navigation(); queue_redraw()
func make_house(p:Vector2i)->void:
	add_building(p,Vector2i(3,0),Vector2i.ONE,"Casa nueva"); features.erase(p); build_navigation(); queue_redraw()

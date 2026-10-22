extends Node2D
const TownMap=preload("res://scripts/town_map.gd")
var town_map:TownMap
var last_elapsed:=0.0
var selected:=0
const Pixel=preload("res://scripts/pixel_text.gd")
const Actors=preload("res://scripts/actors.gd")
const EXT="res://Res/The Fan-tasy Tileset (Exteriores)/Art/"
const INT="res://Res/The Fan-tasy Tileset (Interiores)/Art/"
const GROUND=preload(EXT+"Ground Tileset/Tileset_Ground.png")
const ROAD=preload(EXT+"Ground Tileset/Tileset_Road.png")
const HOUSE=preload(EXT+"Buildings/House_Hay_1.png")
const HOUSE2=preload(EXT+"Buildings/House_Hay_2.png")
const WELL=preload(EXT+"Buildings/Well_Hay_1.png")
const TREE=preload(EXT+"Trees and Bushes/Tree_Emerald_2.png")
const BUSH=preload(EXT+"Trees and Bushes/Bush_Emerald_2.png")
const WALK=preload(EXT+"Characters/Main Character/Character_Walk.png")
const SLASH=preload(EXT+"Characters/Main Character/Character_Slash.png")
const TABLE=preload(INT+"Props/Table_Medium_1.png")
const BED=preload(INT+"Props/HayStack_2.png")
const WATER=preload(INT+"Props/Crate_Water_1.png")
const FIRE=preload(INT+"Props/Fireplace_1.png")
const CRATE=preload(INT+"Props/Crate_Medium_Closed.png")
const SACK=preload(INT+"Props/Sack_3.png")
const BENCH=preload(INT+"Props/Bench_1.png")
const PLANT=preload(INT+"Props/Plant_2.png")
const WALL=preload(INT+"Buildings/House_Hay_1.png")
const DUNGEON=preload("res://Res/Dungeon_Tileset.png")
var sim
var mode:="town"
var town:=0
var resident:=0
var dungeon:=0
var clock:=0.0
var regions:Dictionary={}
var hero_positions:Dictionary={}
var homes:=[Vector2(66,52),Vector2(370,50),Vector2(65,206),Vector2(373,204)]

func region(texture:Texture2D,p:Vector2,source:Rect2,size_px:Vector2,tint:=Color.WHITE)->void:
	draw_texture_rect_region(texture,Rect2(p,size_px),source,tint)
func stamp(texture:Texture2D,p:Vector2,tint:=Color.WHITE)->void:draw_texture(texture,p,tint)
func text_at(p:Vector2,s:String,color:=Color("fff1e8"))->void:Pixel.text(self,p,s,1,color)
func plate(p:Vector2,s:String,color:=Color("fff1e8"))->void:
	Pixel.fill(self,Rect2(p-Vector2(3,3),Vector2(s.length()*6+6,13)),Color("182733"));text_at(p,s,color)
func hit(p:Vector2)->String:
	for id in regions:
		if regions[id].has_point(p):return id
	return ""
func townspeople()->Array:
	return sim.agents.filter(func(a:Dictionary)->bool:return a.home_town==town)
func _draw()->void:
	regions.clear();hero_positions.clear()
	if sim==null:return
	match mode:
		"town":draw_town()
		"house":draw_house()
		"dungeon":draw_dungeon()

func hero(a:Dictionary,p:Vector2,combat:bool=false)->void:
	var frame:int=int(a.frame)%(6 if combat else 4)
	var direction:int=[0,3,2,1][int(a.direction)%4]
	var dims:=Vector2(26,31) if mode=="dungeon" else Vector2(32,38)
	region(SLASH if combat else WALK,p-Vector2(dims.x/2,dims.y*0.81),Rect2(frame*40,direction*48,40,48),dims,Actors.COLORS[int(a.id)])
	hero_positions[str(a.id)]=p;regions["person_%d"%a.id]=Rect2(p-Vector2(12,26),Vector2(24,34))
	if a.id==selected:plate(p+Vector2(-12,10),a.name)

func draw_town()->void:
	if town_map==null:town_map=TownMap.new()
	town_map.floor_to(self)
	var residents:=townspeople()
	var ids:=[4,1,2]
	var sorted:Array=town_map.objects.duplicate()
	var delta:float=clampf(sim.elapsed-last_elapsed,0,8)
	last_elapsed=sim.elapsed
	sorted.append_array(town_map.update(sim,town,delta))
	sorted.sort_custom(func(a:Dictionary,b:Dictionary)->bool:return a.bottom<b.bottom)
	for item in sorted:
		if item.has("agent"):hero(item.agent,item.pos)
		else:town_map.draw_item(self,item)
	for i in residents.size():
		var a:Dictionary=residents[i]
		var rect:Rect2=town_map.houses[ids[i%3]]
		regions["house_%d"%a.id]=rect
		if rect.has_point(get_local_mouse_position()):plate(rect.position+Vector2(0,rect.size.y+3),"CASA DE "+a.name,Color("ffe09b"))
	regions["market"]=town_map.houses[3]
	if residents.size()<3:regions["granary"]=town_map.houses[2]
	regions["granary_store"]=Rect2(142,349,68,55)
	regions["noticeboard"]=Rect2(245,161,45,43)

func draw_house()->void:
	var a:Dictionary=sim.agents[resident]
	Pixel.fill(self,Rect2(0,0,560,336),Color("182330"))
	Pixel.fill(self,Rect2(32,26,496,280),Color("405362"))
	Pixel.fill(self,Rect2(34,28,492,276),Color("202e3b"))
	text_at(Vector2(54,44),"HOGAR DE "+a.name,Color("ffe09b"))
	text_at(Vector2(54,62),"EN CASA" if a.pos.distance_to(sim.world.center(a.home))<10 and a.location=="world" else "FUERA DE CASA",Color("9fb7c5"))
	stamp(HOUSE,Vector2(56,90))
	text_at(Vector2(181,87),"ACTIVIDAD ACTUAL",Color("9fb7c5"))
	text_at(Vector2(181,103),a.thought.left(49))
	text_at(Vector2(181,133),"PROVISIONES",Color("ffe09b"))
	text_at(Vector2(181,151),"GRANO %d / CARNE %d / HUEVOS Y LECHE %d"%[a.grain,a.meat,a.eggs])
	text_at(Vector2(181,170),"SEMILLAS %d / MADERA %d / MONEDAS %d"%[a.seeds,a.wood,a.money])
	text_at(Vector2(181,189),"AGUA GUARDADA %.1f / ENERGIA %d"%[a.home_water,a.energy])
	text_at(Vector2(54,218),"ARTEFACTOS EQUIPADOS",Color("ffe09b"))
	if a.artifacts.is_empty():text_at(Vector2(54,238),"NINGUNO. LAS EXPEDICIONES PUEDEN ENCONTRARLOS.")
	else:
		for i in a.artifacts.size():text_at(Vector2(54+(i%2)*231,238+(i/2)*16),a.artifacts[i].name)
	text_at(Vector2(54,283),"CLIC EN PROVISIONES O ARTEFACTOS PARA INSPECCIONAR",Color("9fb7c5"))
	regions["pantry"]=Rect2(173,126,326,56)
	regions["water"]=Rect2(174,185,180,20)
	regions["bed"]=Rect2(355,185,140,20)
	regions["artifacts"]=Rect2(49,214,460,59)
func draw_dungeon()->void:
	var d=sim.realm.dungeons[dungeon]
	Pixel.fill(self,Rect2(0,0,560,336),Color("171b23"))
	for y in 20:
		for x in 35:
			var p:=Vector2i(x,y)
			if d.tiles.has(p):
				region(DUNGEON,Vector2(p*16),Rect2(160+(x%3)*32,256,32,32),Vector2(16,16),Color("aeaeb8"))
			else:
				var adjacent:=false
				for v in [Vector2i.LEFT,Vector2i.RIGHT,Vector2i.UP,Vector2i.DOWN]:
					if d.tiles.has(p+v):adjacent=true
				if adjacent:region(DUNGEON,Vector2(p*16),Rect2(112,20,32,32),Vector2(16,16),Color("969cac"))
	region(DUNGEON,Vector2(83,56),Rect2(76,589,47,37),Vector2(24,24))
	plate(Vector2(51,34),"ENTRADA",Color("b9cfa4"))
	for chest in d.chests:
		region(DUNGEON,chest.pos-Vector2(8,8),Rect2(10 if not chest.opened else 34,596,23,28),Vector2(16,16),Color("89848a") if chest.opened else Color.WHITE)
	for enemy in d.enemies:
		if enemy.hp<=0:
			region(DUNGEON,enemy.pos-Vector2(6,4),Rect2(69,549,19,19),Vector2(12,10));continue
		var frame:int=int(clock*6)%6 if d.state=="combat" and enemy.room==d.room else 0
		region(SLASH,enemy.pos-Vector2(13,25),Rect2(frame*40,48,40,48),Vector2(26,31),Color("ce6e79") if enemy.kind=="GUARDIAN" else Color("a48abd"))
		Pixel.fill(self,Rect2(enemy.pos+Vector2(-10,-16),Vector2(20,2)),Color("472b39"))
		Pixel.fill(self,Rect2(enemy.pos+Vector2(-10,-16),Vector2(20*enemy.hp/enemy.max_hp,2)),Color("ce7263"))
	for who in d.party:
		if sim.agents[who].location=="dungeon" and d.positions.has(str(who)):
			hero(sim.agents[who],d.positions[str(who)],d.state=="combat")
	plate(Vector2(14,319),"SALAS CONECTADAS / COMBATE AUTOMATICO / BOTIN REAL / RAROS 4%",Color("b7c5d1"))

func inspect(id:String)->String:
	var a:Dictionary=sim.agents[resident]
	match id:
		"bed":return "DESCANSO: %s recupera energia al dormir en su hogar. Energia actual: %d/100."%[a.name,a.energy]
		"water":return "TINAJA: %.1f de agua dulce. Bebe de ella cerca de casa; repone la reserva cuando visita el rio."%a.home_water
		"pantry":return "PROVISIONES DE %s: %d grano, %d carne, %d huevos y leche, %d semillas."%[a.name,a.grain,a.meat,a.eggs,a.seeds]
		"artifacts":
			var names:PackedStringArray=[]
			for item in a.artifacts:names.append(item.name)
			return "ARTEFACTOS EQUIPADOS: "+(", ".join(names) if not names.is_empty() else "NINGUNO. SE ENCUENTRAN EN MAZMORRAS (4%).")
		"noticeboard":return "TABLON COMUNITARIO: abre GESTION para encargar cultivos, caminos, casas o cambiar prioridades."
		"market":return "MERCADO: los habitantes venden madera y artefactos. La aldea recibe 20% del valor del botin."
		"granary", "granary_store":return "GRANERO COMUN: %d de grano. Cosechas reales; se retira comida y se exporta el excedente."%sim.grain_store
	return ""

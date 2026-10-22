extends Node2D
const World = preload("res://scripts/world.gd")
const CHARACTERS = preload("res://Res/Pico-8/MAS-Characters-P8.png")
const CHARACTERS_DB = preload("res://Res/Dawnbringer-16/MAS-Characters-DB.png")
const ANIMALS = preload("res://Res/Pico-8/MAS-Characters-P8.png")
var image_cache: Dictionary = {}
var sim
var selected := 0
var clock := 0.0
const COLORS := [Color(1, 1, 1), Color(1, 0.83, 0.73), Color(0.8, 1, 0.84), Color(0.83, 0.85, 1), Color(1, 0.8, 1), Color(1, 1, 0.68)]

static func sheet_for(a: Dictionary) -> Texture2D:
	return CHARACTERS if int(a.id) % 2 == 0 else CHARACTERS_DB

func frame_cell(a: Dictionary) -> Vector2i:
	var step: int = int(a.frame) % 4 if a.phase == "move" or a.get("action", "") in ["sow", "harvest", "till", "gather", "hunt"] else 0
	var offset := Vector2i((step % 2) * 2, (step / 2) * 2)
	match int(a.direction):
		1: return Vector2i(1, 0) + offset
		2: return Vector2i(0, 1) + offset
		3: return Vector2i(1, 1) + offset
		_: return offset

func tile(texture: Texture2D, p: Vector2, source: Vector2i, tint := Color.WHITE) -> void:
	draw_texture_rect_region(texture, Rect2(p.round(), Vector2(8, 8)), Rect2(Vector2(source * 8), Vector2(8, 8)), tint)

func illumination(p: Vector2) -> Color:
	var day_color: Color = Color(0.3, 0.38, 0.65).lerp(Color.WHITE, sim.daylight())
	for camp in sim.camps:
		var dist: float = p.distance_to(sim.world.center(camp.cell))
		if dist < 35:
			day_color = day_color.lerp(Color(1, 0.81, 0.6), (1.0 - dist / 35.0) * (1.0 - sim.daylight()))
	return day_color

func _draw() -> void:
	if sim == null:
		return
	# Water animation is an existing 8px foam sprite, with sparse placements.
	for y in range(3, World.H, 8):
		for x in range(4, World.W, 13):
			var p := Vector2i(x + int(clock * 0.25) % 2, y)
			if sim.world.tile_at(p) in [World.SEA, World.FRESH]:
				tile(World.TILES, Vector2(p * 8), Vector2i(0, 3), Color(1, 1, 1, 0.35 + sin(clock + x) * 0.12) * illumination(Vector2(p * 8)))
	draw_crops()
	for site in sim.world.dungeon_sites:
		var p:Vector2=Vector2(site.cell*8)
		tile(World.OBJECTS,p-Vector2(8,8),Vector2i(6,2),illumination(p))
		tile(World.OBJECTS,p-Vector2(0,8),Vector2i(7,2),illumination(p))
		tile(World.OBJECTS,p-Vector2(8,0),Vector2i(6,3),illumination(p))
		tile(World.OBJECTS,p,Vector2i(7,3),illumination(p))
	for ship in sim.realm.ships:
		var p:Vector2=sim.world.center(ship.cell)+Vector2(12,25)
		if ship.remaining>0:p+=Vector2(18,75)*sin(ship.remaining/60.0*PI)
		tile(World.OBJECTS,p,Vector2i(6,0),illumination(p))
	for job in sim.realm.jobs:
		tile(World.OBJECTS,Vector2(job.cell*8),Vector2i(2,0),Color("ffc37e"))
	for p in sim.realm.protected.values():
		if (p.x+p.y)%5==0:tile(World.OBJECTS,Vector2(p*8),Vector2i(2,0),Color("88eebb"))
	# Pasture fencing is built from the supplied fence sprites.
	for x in range(28,35):
		tile(World.FENCES,Vector2(x*8,55*8),Vector2i(1,0),illumination(Vector2(x*8,55*8)))
		tile(World.FENCES,Vector2(x*8,61*8),Vector2i(1,0),illumination(Vector2(x*8,61*8)))
	for camp in sim.camps:
		var p: Vector2 = Vector2(camp.cell * 8)
		tile(World.OBJECTS, p + Vector2(-8, -8), Vector2i(3, 0), illumination(p))
		# Existing fire and smoke cells from the terrain atlas; no particle or light resources.
		cutout(World.TILES, p + Vector2(8, 0), Vector2i(12, 8), Color(1, 0.82 + sin(clock * 6) * 0.12, 0.8))
		if int(clock * 2) % 3 != 0:
			cutout(World.TILES, p + Vector2(8, -8), Vector2i(12, 7), illumination(p))
	for animal in sim.animals:
		if animal.alive:
			tile(ANIMALS, animal.pos - Vector2(4, 4), Vector2i(animal.kind, 4), illumination(animal.pos))
	var sorted: Array = sim.agents.duplicate()
	sorted.sort_custom(func(a: Dictionary, b: Dictionary) -> bool: return a.pos.y < b.pos.y)
	for a in sorted:
		if a.location!="world":continue
		var p: Vector2 = a.pos - Vector2(4, 4)
		if not a.alive:
			tile(World.TILES, p, Vector2i(6, 15), illumination(a.pos))
			continue
		tile(sheet_for(a), p, frame_cell(a), illumination(a.pos))

func cutout(texture:Texture2D,p:Vector2,source:Vector2i,tint:Color)->void:
	if not image_cache.has(texture): image_cache[texture]=texture.get_image()
	var picture:Image=image_cache[texture]
	var green:Color=World.TILES.get_image().get_pixel(8,0)
	for y in 8:
		for x in 8:
			var at:=source*8+Vector2i(x,y)
			var color:=picture.get_pixelv(at)
			if color.a<0.1 or color.is_equal_approx(green): continue
			draw_texture_rect_region(texture,Rect2(p.round()+Vector2(x,y),Vector2.ONE),Rect2(Vector2(at),Vector2.ONE),tint)

func draw_crops()->void:
	for crop in sim.crops:
		var p:Vector2=Vector2(crop.cell*8)
		var light:=illumination(p)
		# All four stages use the supplied field tiles; harvested ground stays visibly bare.
		tile(World.FIELDS,p,Vector2i(4,0),light)
		if crop.stage==0: continue
		if crop.stage==1:
			draw_texture_rect_region(World.FIELDS,Rect2(p+Vector2(0,5),Vector2(8,3)),Rect2(8,8,8,8),light*Color(0.48,0.85,0.5))
		else:
			var color:=Color(0.52,0.94,0.58) if crop.stage==2 else Color.WHITE
			tile(World.FIELDS,p,Vector2i(1+int(crop.cell.x)%3,1),light*color)

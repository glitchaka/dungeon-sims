extends RefCounted
## Fixed-step autonomous survival simulation. No player-directed movement.
const World = preload("res://scripts/world.gd")
const Realm = preload("res://scripts/realm.gd")
const Persistence = preload("res://scripts/persistence.gd")
var realm: Realm
const STEP := 0.2
const DAY_SECONDS := 300.0
const NEEDS := ["hunger", "thirst", "energy", "social", "fun", "hygiene"]
var world: World
var rng := RandomNumberGenerator.new()
var agents: Array[Dictionary] = []
var animals: Array[Dictionary] = []
var camps: Array[Dictionary] = []
var crops: Array[Dictionary] = []
var grain_store := 0
var events: Array[String] = []
var metrics := {"hunt": 0, "eat": 0, "drink": 0, "gather": 0, "camp": 0, "sleep": 0, "social": 0, "work": 0, "bathe": 0, "explore": 0, "sow": 0, "harvest": 0, "till": 0, "deposit": 0, "fetch": 0, "spoil": 0, "grain_eaten": 0}
var elapsed := DAY_SECONDS * 0.31
var accumulator := 0.0
var next_id := 0

func setup(map: World) -> void:
	world = map
	realm = Realm.new(self)
	for metric in ["project","trade","expeditions","dungeon_kills","artifacts_found","rare_artifacts","sell_loot"]:metrics[metric]=0
	rng.seed = 91822
	var names := ["Luna", "Bruno", "Alma", "Teo", "Vera", "Nico"]
	var traits := ["Exploradora", "Sociable", "Cazadora", "Trabajador", "Agricultora", "Explorador"]
	for i in names.size():
		var p := World.HOME + Vector2i(i % 3, i / 3)
		var a := {"id": i, "name": names[i], "trait": traits[i], "pos": world.center(p), "goal": p, "path": PackedVector2Array(), "path_index": 0, "action": "idle", "phase": "idle", "timer": 0.0, "repath": 0.0, "target": -1, "direction": 0, "frame": 0.0, "wood": 0, "meat": 0, "grain": 0, "seeds": 12, "money": 12, "health": 100.0, "alive": true, "relationships": {}, "resume": "", "thought": "Observando la isla", "hunger": 34.0 + i * 7, "thirst": 49.0 + i * 5, "energy": 64.0 + i * 4, "social": 55.0 + i * 4, "fun": 44.0 + i * 6, "hygiene": 80.0}
		if i in [3,4]:
			a.hunger=78.0; a.thirst=82.0; a.energy=90.0
		a.location="world";a.mission=-1;a.loot=[];a.artifacts=[];a.home=World.REST_SPOTS[i];a.home_town=0 if i<3 else (1 if i<5 else 2);a.job=-1
		a.home_water=4.0;a.eggs=0;a.pos=world.center(a.home);a.goal=a.home
		agents.append(a)
	for p in world.initial_fields:
		var stage := 3 if p.x < 41 else (2 if p.y < 59 else 0)
		crops.append({"cell": p, "stage": stage, "growth": 120.0 if stage == 3 else (65.0 if stage == 2 else 0.0), "ripe_age": 0.0, "cycles": 0})
	for i in 20:
		spawn_animal()
	record("La aldea despierta. Cada habitante decide por si mismo.")

func hour() -> float:
	return fmod(elapsed / DAY_SECONDS * 24.0, 24.0)

func day() -> int:
	return int(elapsed / DAY_SECONDS) + 1

func night() -> bool:
	return hour() >= 19.0 or hour() < 6.0

func daylight() -> float:
	return clampf((sin((hour() - 6.0) / 24.0 * TAU) + 0.25) / 0.65, 0.0, 1.0)

func advance(delta: float) -> void:
	accumulator += delta
	while accumulator >= STEP:
		accumulator -= STEP
		tick(STEP)

func tick(dt: float) -> void:
	elapsed += dt
	update_crops(dt)
	realm.tick(dt)
	for animal in animals:
		update_animal(animal, dt)
	for a in agents:
		if not a.alive or a.location=="dungeon":
			continue
		a.hunger = maxf(0, a.hunger - dt * 0.066)
		a.thirst = maxf(0, a.thirst - dt * (0.11 if a.phase == "move" else 0.085) * (0.75 if has_bonus(a,"thirst") else 1.0))
		a.energy = maxf(0, a.energy - dt * (0.075 if a.phase == "move" else 0.038))
		a.social = maxf(0, a.social - dt * (0.06 if a.trait == "Sociable" else 0.037))
		a.fun = maxf(0, a.fun - dt * 0.045)
		a.hygiene = maxf(0, a.hygiene - dt * (0.07 if a.action == "hunt" else 0.035))
		if a.hunger < 2 or a.thirst < 2:
			a.health = maxf(0, a.health - dt * 0.22)
		elif a.hunger > 30 and a.thirst > 30:
			a.health = minf(100, a.health + dt * 0.18)
		if a.health <= 0:
			a.alive = false
			a.thought = "Ha fallecido"
			record(a.name + " no ha sobrevivido.")
			continue
		# Urgent needs can interrupt travel, work, leisure and sleep.
		if a.thirst < 16 and a.action != "drink":
			begin(a, "drink", world.nearest(a.pos, world.water_spots), "Necesito agua dulce")
		elif a.hunger < 14 and a.action not in ["hunt", "eat", "drink", "fetch", "harvest"]:
			feed(a)
		if a.phase == "idle":
			decide(a)
		elif a.phase == "move":
			travel(a, dt)
		elif a.phase == "act":
			perform(a, dt)

func has_bonus(a:Dictionary,bonus:String)->bool:
	for item in a.artifacts:
		if item.bonus==bonus:return true
	return false

func decide(a: Dictionary) -> void:
	if a.mission>=0 and a.hunger>22 and a.thirst>25 and a.energy>15:
		begin(a,"expedition",realm.dungeons[a.mission].entrance,"Viajando a la mazmorra")
		return
	if not a.loot.is_empty() and a.hunger>28 and a.thirst>30 and a.energy>25:
		begin(a,"sell_loot",World.MARKET,"Llevando artefactos al mercado")
		return
	if realm.choose_job(a):return
	if a.thirst<65 and a.home_water>1 and a.pos.distance_to(world.center(a.home))<48:
		begin(a,"drink_home",a.home,"Bebiendo de mi tinaja de agua dulce");return
	if not a.resume.is_empty():
		var resume: String = a.resume
		a.resume = ""
		if resume == "sleep":
			rest(a)
			return
	if a.grain >= 6 and a.thirst > 30 and a.hunger > 30 and a.energy > 30:
		begin(a, "deposit", World.GRANARY, "Llevando la cosecha al granero")
		return
	var rest_bonus := 28.0 if night() else 0.0
	var scores := {"drink": (100.0 - a.thirst) * 1.28, "food": (100.0 - a.hunger) * 1.1, "sleep": (100.0 - a.energy) * 1.05 + rest_bonus, "social": (100.0 - a.social) * 0.8, "explore": (100.0 - a.fun) * 0.86, "bathe": (100.0 - a.hygiene) * 0.82, "work": 38.0 if hour() > 8 and hour() < 17 else 0.0}
	if a.thirst < 30:
		scores.drink += 60
	if a.hunger < 27:
		scores.food += 55
	if a.energy < 25:
		scores.sleep += 60
	if a.trait.begins_with("Explorador"):
		scores.explore += 8
	if a.trait == "Sociable":
		scores.social += 8
	if a.trait == "Trabajador" and a.money < 80:
		scores.work += 9
	if a.trait == "Cazadora":
		scores.food += 6
	var farm: Dictionary = farm_task(a)
	if not farm.is_empty() and not night():
		scores["farm"] = 54.0 if a.trait in ["Agricultora", "Trabajador"] else 31.0
		if farm.action == "harvest" and grain_store < 12: scores.farm += 13
		if realm.priority=="food":scores.farm+=20
	if realm.priority=="build":scores.work+=18
	var choice := "explore"
	var best := -1.0
	for key in scores:
		if scores[key] > best:
			best = scores[key]
			choice = key
	match choice:
		"farm": begin(a, farm.action, farm.cell, {"harvest": "Cosechando trigo maduro", "sow": "Sembrando trigo con mis semillas", "till": "Abriendo una parcela nueva"}[farm.action])
		"drink": begin(a, "drink", world.nearest(a.pos, world.water_spots), "Buscando una orilla para beber")
		"food": feed(a)
		"sleep": rest(a)
		"bathe": begin(a, "bathe", world.nearest(a.pos, world.water_spots), "Voy a lavarme en el rio")
		"social": socialize(a)
		"work":
			if realm.forest_options().is_empty():
				socialize(a)
			elif a.wood < 4:
				begin(a, "gather", world.nearest(a.pos, realm.forest_options()), "Recolectando madera para la aldea")
			else:
				begin(a, "work", World.MARKET, "Llevando madera al mercado")
		_:
			var target: Vector2i = world.forest[rng.randi_range(0, world.forest.size() - 1)]
			begin(a, "explore", target, "Explorando el bosque")

func feed(a: Dictionary) -> void:
	if a.meat > 0 or a.grain > 0 or a.eggs>0:
		begin(a, "eat", a.home if a.pos.distance_to(world.center(a.home))<48 else world.cell(a.pos), "Comiendo mis provisiones")
		return
	if realm.livestock_food>0 and a.trait!="Cazadora" and a.pos.distance_to(world.center(World.HOME))<130:
		begin(a,"tend",world.nearest(a.pos,world.pasture),"Cuidando el ganado y recogiendo huevos");return
	if grain_store > 0 and a.trait != "Cazadora":
		begin(a, "fetch", World.GRANARY, "Buscando alimento en el granero")
		return
	var best := -1
	var distance := INF
	for i in animals.size():
		var prey: Dictionary = animals[i]
		if not prey.alive or prey.kind!=4 or realm.protected.has(realm.key(world.cell(prey.pos))):
			continue
		var reserved := false
		for other in agents:
			if other.id != a.id and other.action == "hunt" and other.target == i:
				reserved = true
		if reserved:
			continue
		var d: float = a.pos.distance_squared_to(prey.pos)
		if d < distance:
			distance = d
			best = i
	if best >= 0:
		a.target = best
		begin(a, "hunt", world.cell(animals[best].pos), "Buscando una presa en el bosque")
	else:
		# Population recovers naturally; no food is created by walking into a forest.
		begin(a, "explore", world.nearest(a.pos, realm.forest_options()), "Buscando rastros; las presas escasean")

func rest(a: Dictionary) -> void:
	var village_distance: float = a.pos.distance_to(world.center(a.home))
	if village_distance <= 88:
		begin(a, "sleep", a.home, "Volviendo a la aldea para descansar")
		return
	for camp in camps:
		if a.pos.distance_to(world.center(camp.cell)) < 64 and (camp.owner == a.id or not camp_occupied(camp.cell)):
			begin(a, "sleep", camp.cell, "Buscando refugio en un campamento")
			return
	if a.wood < 3:
		if realm.forest_options().is_empty():
			begin(a,"sleep",a.home,"Los bosques estan protegidos; regreso a casa");return
		a.resume = "sleep"
		begin(a, "gather", world.nearest(a.pos, realm.forest_options()), "Necesito madera para levantar un campamento")
	else:
		var spot := world.cell(a.pos)
		if world.tile_at(spot) in [World.ROAD, World.BRIDGE] or camp_occupied(spot):
			var candidates: Array[Vector2i] = []
			for p in world.accessible:
				if world.tile_at(p) == World.LAND and not camp_occupied(p):
					candidates.append(p)
			spot = world.nearest(a.pos, candidates)
		begin(a, "camp", spot, "Levantando un refugio lejos de la aldea")

func camp_occupied(p: Vector2i) -> bool:
	for a in agents:
		if a.action == "sleep" and a.goal == p:
			return true
	return false

func socialize(a: Dictionary) -> void:
	var best := -1
	var distance := INF
	for other in agents:
		if other.id == a.id or not other.alive or other.location!="world" or other.mission>=0 or other.action in ["hunt", "sleep", "drink", "eat"]:
			continue
		var d: float = a.pos.distance_squared_to(other.pos)
		if d < distance:
			distance = d
			best = other.id
	if best >= 0:
		a.target = best
		begin(a, "social", world.cell(agents[best].pos), "Quiero conversar con " + agents[best].name)
	else:
		begin(a, "explore", world.nearest(a.pos, realm.forest_options()), "Disfrutando un paseo tranquilo")

func begin(a: Dictionary, action: String, target_cell: Vector2i, thought: String) -> void:
	a.action = action
	a.thought = thought
	a.goal = target_cell
	a.path = world.path(a.pos, target_cell)
	a.path_index = 0
	a.repath = 0.0
	a.timer = 0.0
	if a.path.is_empty():
		a.phase = "idle"
		a.resume = ""
		return
	# Skip the start-cell center only when already centered. This preserves collisions.
	if a.pos.distance_to(a.path[0]) < 0.1:
		a.path_index = 1
	a.phase = "move"

func travel(a: Dictionary, dt: float) -> void:
	a.repath += dt
	if a.action in ["hunt", "social"]:
		var target_pos: Vector2
		if a.action == "hunt":
			if a.target < 0 or not animals[a.target].alive:
				a.phase = "idle"
				return
			target_pos = animals[a.target].pos
		else:
			if a.target < 0 or not agents[a.target].alive:
				a.phase = "idle"
				return
			target_pos = agents[a.target].pos
		if a.pos.distance_to(target_pos) <= 9:
			a.phase = "act"
			a.timer = 0
			return
		if a.repath >= 1.8:
			a.goal = world.cell(target_pos)
			a.path = world.path(a.pos, a.goal)
			a.path_index = 0
			a.repath = 0
	var budget := dt * (14.0 if a.energy > 15 else 9.0) * (1.15 if has_bonus(a,"speed") else 1.0)
	while budget > 0 and a.path_index < a.path.size():
		var target: Vector2 = a.path[a.path_index]
		if not world.walkable(world.cell(target)):
			begin(a,a.action,a.goal,a.thought);return
		var diff: Vector2 = target - a.pos
		var distance := diff.length()
		if distance > 0.01:
			a.direction = (1 if diff.x > 0 else 3) if abs(diff.x) > abs(diff.y) else (0 if diff.y > 0 else 2)
		if distance <= budget:
			a.pos = target
			budget -= distance
			a.path_index += 1
		else:
			a.pos += diff.normalized() * budget
			budget = 0
	a.frame += dt * 7
	if a.path_index >= a.path.size():
		a.phase = "act"
		a.timer = 0

func perform(a: Dictionary, dt: float) -> void:
	a.timer += dt
	a.frame += dt * 4
	match a.action:
		"tend":
			if a.timer>=5:
				var amount:=mini(3,realm.livestock_food)
				a.eggs+=amount;realm.livestock_food-=amount
				finish(a,"tend","recogio %d raciones de huevos y leche"%amount)
		"drink_home":
			a.thirst=minf(100,a.thirst+dt*9);a.home_water=maxf(0,a.home_water-dt*0.3)
			if a.thirst>=98 or a.home_water<=0:finish(a,"drink","bebio de su reserva de agua dulce")
		"project":
			if a.timer>=8:realm.complete_job(a)
		"expedition":
			a.thought="Esperando a mis companeros en la entrada"
		"sell_loot":
			if a.timer>=4:
				var value:=0
				for item in a.loot:value+=int(item.value)
				var tax:=int(value*0.2)
				a.money+=value-tax;realm.treasury+=tax;a.loot.clear()
				finish(a,"sell_loot","vendio sus artefactos por %d monedas"%value)
		"drink":
			assert(world.water_spots.has(world.cell(a.pos)), "Drinking must happen at a freshwater bank")
			a.thirst = minf(100, a.thirst + dt * 9)
			if a.thirst >= 98:
				a.home_water=4.0
				finish(a, "drink", "bebio agua dulce")
		"hunt":
			var prey: Dictionary = animals[a.target]
			if realm.protected.has(realm.key(world.cell(prey.pos))):
				a.phase="idle";return
			if not prey.alive or prey.kind!=4 or realm.protected.has(realm.key(world.cell(prey.pos))):
				a.phase = "idle"
			elif a.pos.distance_to(prey.pos) > 11:
				begin(a, "hunt", world.cell(prey.pos), "Siguiendo a la presa")
			elif a.timer >= 2.6:
				prey.alive = false
				prey.respawn = 180.0
				a.meat += 3
				a.energy = maxf(0, a.energy - 3)
				a.fun = minf(100, a.fun + 9)
				finish(a, "hunt", "cazo una presa y obtuvo 3 raciones")
		"eat":
			if a.timer >= 4:
				if a.eggs>0:
					a.eggs-=1;a.hunger=minf(100,a.hunger+36)
				elif a.grain > 0:
					a.grain -= 1
					metrics.grain_eaten += 1
					a.hunger = minf(100, a.hunger + 42)
				elif a.meat > 0:
					a.meat -= 1
					a.hunger = minf(100, a.hunger + 54)
				finish(a, "eat", "comio una racion de sus provisiones")
		"sow", "harvest", "till":
			if a.timer >= (6.0 if a.action == "till" else 4.0):
				finish_farm(a)
		"deposit":
			if a.timer >= 3:
				var amount: int = maxi(0, a.grain - 2)
				a.grain -= amount
				grain_store += amount
				a.money += amount
				finish(a, "deposit", "guardo %d de grano para la aldea" % amount)
		"fetch":
			if a.timer >= 2:
				var amount: int = mini(3, grain_store)
				a.grain += amount
				grain_store -= amount
				finish(a, "fetch", "retiro %d de grano del granero" % amount)
		"gather":
			if not world.forest.has(a.goal) or realm.protected.has(realm.key(a.goal)):
				a.phase="idle";a.resume="";return
			if a.timer >= 8:
				a.wood += 4
				finish(a, "gather", "recolecto 4 unidades de madera")
		"camp":
			if a.timer >= 7:
				if a.wood >= 3:
					a.wood -= 3
					camps=camps.filter(func(c:Dictionary)->bool:return c.owner!=a.id)
					camps.append({"cell": a.goal, "owner": a.id, "created": elapsed})
					finish(a, "camp", "levanto un campamento con 3 de madera")
					begin(a, "sleep", a.goal, "Durmiendo protegido en el campamento")
				else:
					a.phase = "idle"
		"sleep":
			a.energy = minf(100, a.energy + dt * 1.3 * (1.25 if has_bonus(a,"rest") else 1.0))
			if not night() and a.timer > 18 and a.energy > 78:
				finish(a, "sleep", "desperto con energia renovada")
		"bathe":
			a.hygiene = minf(100, a.hygiene + dt * 7)
			if a.hygiene >= 98:
				finish(a, "bathe", "se lavo en la orilla")
		"social":
			var other: Dictionary = agents[a.target]
			if not other.alive or a.pos.distance_to(other.pos) > 16:
				if a.timer > 2:
					a.phase = "idle"
				return
			a.social = minf(100, a.social + dt * 5)
			other.social = minf(100, other.social + dt * 3)
			if a.timer >= 7:
				var key := str(other.id)
				a.relationships[key] = mini(100, a.relationships.get(key, 0) + 8)
				other.relationships[str(a.id)] = mini(100, other.relationships.get(str(a.id), 0) + 8)
				finish(a, "social", "converso con " + other.name)
		"work":
			if a.timer >= 10:
				var sold: int = mini(a.wood, 4)
				a.wood -= sold
				a.money += sold * 3
				realm.wood+=sold
				finish(a, "work", "vendio madera en el mercado")
		"explore":
			a.fun = minf(100, a.fun + dt * 3)
			if a.timer >= 12:
				finish(a, "explore", "disfruto del bosque")
		_:
			a.phase = "idle"

func finish(a: Dictionary, action: String, description: String) -> void:
	metrics[action] += 1
	record(a.name + " " + description + ".")
	a.phase = "idle"
	a.action = "idle"
	a.timer = 0.0

func record(message: String) -> void:
	var stamp := "D%d %02d:%02d" % [day(), int(hour()), int(fmod(hour(), 1.0) * 60)]
	events.append(stamp + " | " + message)
	if events.size() > 120:
		events.pop_front()

func spawn_animal() -> void:
	var p: Vector2i = world.forest[rng.randi_range(0, world.forest.size() - 1)]
	if next_id<8:p=world.pasture[next_id%world.pasture.size()]
	animals.append({"id": next_id, "pos": world.center(p), "kind": 4 if next_id>=8 else [0,1,2,3,5][next_id%5], "alive": true, "timer": rng.randf_range(1, 6), "respawn": 0.0})
	next_id += 1

func update_animal(animal: Dictionary, dt: float) -> void:
	if not animal.alive:
		animal.respawn -= dt
		if animal.respawn <= 0:
			animal.pos = world.center(world.forest[rng.randi_range(0, world.forest.size() - 1)])
			animal.alive = true
		return
	animal.timer -= dt
	if animal.timer > 0:
		return
	animal.timer = rng.randf_range(2, 6)
	for a in agents:
		if a.action == "hunt" and a.target == animal.id and a.pos.distance_to(animal.pos) < 13:
			return
	var directions := [Vector2i.LEFT, Vector2i.RIGHT, Vector2i.UP, Vector2i.DOWN]
	var p: Vector2i = world.cell(animal.pos) + directions[rng.randi_range(0, 3)]
	if (world.forest.has(p) if animal.kind==4 else world.pasture.has(p)) and world.walkable(p):
		animal.pos = world.center(p)

func save_state(path:String="user://isla_viva_v3.json")->Error:
	var data:Dictionary={"version":3,"elapsed":elapsed,"rng":str(rng.state),"agents":agents,"animals":animals,"camps":camps,"metrics":metrics,"events":events,"crops":crops,"grain_store":grain_store,"realm":realm.snapshot()}
	var file:=FileAccess.open(path,FileAccess.WRITE)
	if file==null:return FileAccess.get_open_error()
	file.store_string(JSON.stringify(Persistence.encode(data),"",true,true))
	return OK

func load_state(path:String="user://isla_viva_v3.json")->bool:
	if not FileAccess.file_exists(path):return false
	var raw=JSON.parse_string(FileAccess.get_file_as_string(path))
	if not raw is Dictionary or raw.get("version",0)!=3 or not raw.has_all(["elapsed","agents","animals","camps","metrics","events","crops","grain_store","realm","rng"]):return false
	var data:Dictionary=Persistence.decode(raw)
	if data.agents.size()!=6:return false
	for a in data.agents:
		if not a.has_all(["location","mission","home","loot","artifacts","pos","goal","path"]+NEEDS):return false
	world.reset_world()
	realm=Realm.new(self);realm.restore(data.realm)
	agents.assign(data.agents)
	for a in agents:
		a.id=int(a.id);a.mission=int(a.mission);a.target=int(a.target);a.direction=int(a.direction)
		a.path=PackedVector2Array(a.path);a.path_index=int(a.path_index)
	animals.assign(data.animals);camps.assign(data.camps);crops.assign(data.crops)
	for animal in animals:animal.id=int(animal.id);animal.kind=int(animal.kind)
	for crop in crops:crop.stage=int(crop.stage)
	metrics=data.metrics;events.assign(data.events);grain_store=int(data.grain_store)
	elapsed=data.elapsed;rng.state=int(data.rng);next_id=animals.size();accumulator=0
	record("Partida recuperada, incluidas las expediciones.")
	return true
func crop_at(p: Vector2i) -> Dictionary:
	for crop in crops:
		if crop.cell == p: return crop
	return {}

func farm_reserved(p: Vector2i, except_id: int) -> bool:
	for a in agents:
		if a.id != except_id and a.phase != "idle" and a.action in ["till", "sow", "harvest"] and a.goal == p: return true
	return false

func farm_task(a: Dictionary) -> Dictionary:
	# A farmer grows the cultivated area gradually as the village establishes reserves.
	var desired := mini(world.initial_fields.size()+24,world.initial_fields.size()+day()*2)
	if crops.size()<desired and metrics.harvest>0 and a.seeds>2 and a.trait in ["Agricultora","Trabajador"]:
		var expansion := expansion_task(a)
		if not expansion.is_empty(): return expansion
	var choice: Dictionary = {}
	var best := INF
	for crop in crops:
		if farm_reserved(crop.cell, a.id): continue
		var action := ""
		if crop.stage == 3 and (grain_store < 160 or a.seeds < 6): action = "harvest"
		elif crop.stage == 0 and a.seeds > 0 and grain_store < 130: action = "sow"
		if action.is_empty(): continue
		var score: float = a.pos.distance_to(world.center(crop.cell)) + (0 if action == "harvest" else 65)
		if score < best:
			best = score
			choice = {"cell": crop.cell, "action": action}
	if not choice.is_empty(): return choice
	# Expand onto nearby usable ground only; never plant over roads, water or buildings.
	if crops.size() < world.initial_fields.size() + 24 and a.seeds > 1 and grain_store < 100:
		for p in world.farm_sites:
			if not crop_at(p).is_empty() or farm_reserved(p,a.id) or realm.fallow.has(realm.key(p)) or realm.protected.has(realm.key(p)): continue
			var near_field := false
			for crop in crops:
				if Vector2(crop.cell-p).length() <= 1.5: near_field = true; break
			if not near_field: continue
			var distance: float = a.pos.distance_to(world.center(p))
			if distance < best:
				best = distance
				choice = {"cell": p, "action": "till"}
	return choice

func update_crops(dt: float) -> void:
	for crop in crops:
		if crop.stage in [1,2]:
			crop.growth += dt * (0.3 if night() else 1.0)
			if crop.growth >= 120:
				crop.stage = 3
				crop.ripe_age = 0.0
			elif crop.growth >= 40: crop.stage = 2
		elif crop.stage == 3:
			crop.ripe_age += dt
			if crop.ripe_age > 300:
				crop.stage = 0
				crop.growth = 0.0
				metrics.spoil += 1

func finish_farm(a: Dictionary) -> void:
	var crop := crop_at(a.goal)
	if world.cell(a.pos) != a.goal:
		a.phase = "idle"
		return
	match a.action:
		"till":
			if crop.is_empty() and world.farm_sites.has(a.goal) and crops.size() < world.initial_fields.size()+24:
				crops.append({"cell":a.goal,"stage":0,"growth":0.0,"ripe_age":0.0,"cycles":0})
				finish(a,"till","preparo una parcela nueva")
			else: a.phase = "idle"
		"sow":
			if not crop.is_empty() and crop.stage == 0 and a.seeds > 0:
				a.seeds -= 1
				crop.stage = 1
				crop.growth = 0.0
				finish(a,"sow","sembro trigo usando una semilla")
			else: a.phase = "idle"
		"harvest":
			if not crop.is_empty() and crop.stage == 3:
				a.grain += 4 if has_bonus(a,"harvest") else 3
				a.seeds += 2
				crop.stage = 0
				crop.growth = 0.0
				crop.cycles += 1
				finish(a,"harvest","cosecho 3 de grano y 2 semillas")
			else: a.phase = "idle"
func expansion_task(a:Dictionary)->Dictionary:
	var best:=INF
	var result:Dictionary={}
	for p in world.farm_sites:
		if not crop_at(p).is_empty() or farm_reserved(p,a.id) or realm.fallow.has(realm.key(p)) or realm.protected.has(realm.key(p)): continue
		var neighbor:=false
		for crop in crops:
			if Vector2(crop.cell-p).length()<=1.5: neighbor=true; break
		if not neighbor: continue
		var distance:float=a.pos.distance_to(world.center(p))
		if distance<best:
			best=distance
			result={"cell":p,"action":"till"}
	return result

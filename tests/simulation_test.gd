extends SceneTree
const World = preload("res://scripts/world.gd")
const Simulation = preload("res://scripts/simulation.gd")
var failed := 0
var checks := 0

func _initialize() -> void:
	run.call_deferred()

func check(condition: bool, description: String) -> void:
	checks += 1
	if not condition:
		failed += 1
		printerr("FAIL: " + description)
	else:
		print("PASS: " + description)

func run() -> void:
	var world := World.new()
	root.add_child(world)
	world.generate()
	check(world.accessible.size() > 600, "Large connected navigable island")
	check(world.forest.size() > 100, "Reachable woodland clearings")
	check(world.water_spots.size() > 20, "Reachable freshwater banks")
	check(not world.walkable(Vector2i(0, 0)), "Ocean blocks navigation")
	check(not world.walkable(World.LAKE), "Lake blocks navigation")
	var sim := Simulation.new()
	sim.setup(world)
	var a: Dictionary = sim.agents[0]
	a.trait="Cazadora" # Explicit hunting scenario; other residents may now choose farm food.
	a.hunger = 6.0
	a.thirst = 100.0
	a.energy = 95.0
	for i in 900:
		sim.tick(0.2)
		if a.hunger > 50:
			break
	check(a.hunger > 50 and a.meat > 0, "Hungry agent autonomously hunts and eats actual inventory")
	check(sim.metrics.hunt > 0 and sim.metrics.eat > 0, "Successful hunting and consumption recorded")
	a.thirst = 3.0
	for i in 800:
		sim.tick(0.2)
		if a.thirst > 95:
			break
	check(a.thirst > 95 and world.water_spots.has(world.cell(a.pos)), "Thirst interrupts current action and is restored at freshwater only")
	var far := World.HOME
	for p in world.forest:
		if world.center(p).distance_to(world.center(World.HOME)) > 170:
			far = p
			break
	check(far != World.HOME, "Remote woodland exists for camping")
	a.pos = world.center(far)
	a.energy = 5.0
	a.hunger = 100.0
	a.thirst = 100.0
	a.wood = 0
	a.phase = "idle"
	a.action = "idle"
	a.resume = ""
	var camp_count := sim.camps.size()
	for i in 650:
		sim.tick(0.2)
		if sim.camps.size() > camp_count and a.energy > 65:
			break
	check(sim.camps.size() > camp_count, "Remote tired agent gathers wood and builds a camp autonomously")
	check(a.wood == 1, "Camp consumes exactly 3 of 4 gathered wood")
	check(a.energy > 65, "Sleeping at camp restores energy")
	sim.elapsed = 0
	var dark := sim.daylight()
	sim.elapsed = Simulation.DAY_SECONDS * 0.5
	check(dark < 0.1 and sim.daylight() > 0.95, "Day and night have distinct illumination")
	sim.elapsed = Simulation.DAY_SECONDS * 0.24
	var dawn_before := sim.daylight()
	sim.elapsed = Simulation.DAY_SECONDS * 0.26
	check(sim.daylight() > dawn_before, "Dawn changes illumination gradually")
	var save_path := "user://verification_save.json"
	check(sim.save_state(save_path) == OK, "Save writes successfully")
	var loaded := Simulation.new()
	loaded.setup(world)
	check(loaded.load_state(save_path), "Save loads successfully")
	check(loaded.agents[0].pos == a.pos and loaded.agents[0].wood == a.wood and loaded.camps.size() == sim.camps.size(), "Persistence retains positions, inventory and camps")
	for i in 50:
		loaded.tick(0.2)
	check(loaded.elapsed > sim.elapsed, "Loaded autonomous simulation resumes")
	var live := Simulation.new()
	live.setup(world)
	var navigable := true
	for i in 18000:
		live.tick(0.2)
		for inhabitant in live.agents:
			if not world.walkable(world.cell(inhabitant.pos)):
				navigable = false
	var survivors := 0
	for inhabitant in live.agents:
		if inhabitant.alive:
			survivors += 1
		print("AGENT ", inhabitant.name, " health=", inhabitant.health, " needs=", [inhabitant.hunger, inhabitant.thirst, inhabitant.energy], " action=", inhabitant.action, " money=", inhabitant.money)
	check(navigable, "No agent crosses blocked terrain during 12 simulated days")
	check(survivors == 6, "All six autonomous inhabitants survive 12 simulated days")
	for action in ["hunt", "eat", "drink", "camp", "sleep", "gather", "explore", "social", "work", "bathe", "harvest", "sow", "till", "deposit", "fetch", "grain_eaten"]:
		check(live.metrics[action] > 0, "Emergent action occurs: " + action)
	print("METRICS ", JSON.stringify(live.metrics))
	print("RESULT ", checks - failed, "/", checks, " passed")
	world.queue_free()
	quit(1 if failed > 0 else 0)

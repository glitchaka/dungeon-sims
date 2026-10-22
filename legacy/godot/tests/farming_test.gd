extends SceneTree
const World = preload("res://scripts/world.gd")
const Simulation = preload("res://scripts/simulation.gd")
var failed:=0
var checks:=0
func _initialize()->void: run.call_deferred()
func check(ok:bool,description:String)->void:
	checks+=1
	if not ok: failed+=1; printerr("FAIL: ",description)
	else: print("PASS: ",description)
func run()->void:
	var world:=World.new(); root.add_child(world); world.generate()
	var sim:=Simulation.new(); sim.setup(world)
	var a:Dictionary=sim.agents[3]
	var crop:Dictionary=sim.crops[0]
	crop.stage=0; crop.growth=0.0
	a.pos=world.center(crop.cell); a.goal=crop.cell; a.seeds=1; a.action="sow"
	sim.finish_farm(a)
	check(a.seeds==0 and crop.stage==1,"Sowing consumes one actual seed")
	var before_grain:int=a.grain
	a.action="harvest"; sim.finish_farm(a)
	check(a.grain==before_grain and crop.stage==1,"Immature wheat cannot be harvested")
	sim.elapsed=Simulation.DAY_SECONDS*0.5
	sim.update_crops(41)
	check(crop.stage==2,"Seeded plot visibly becomes growing wheat")
	sim.update_crops(80)
	check(crop.stage==3,"Growth produces ripe wheat")
	a.action="harvest"; sim.finish_farm(a)
	check(a.grain==before_grain+3 and a.seeds==2,"Harvest yields exactly 3 grain and 2 seeds")
	check(crop.stage==0 and crop.cycles==1,"Harvest removes the standing crop and records a cycle")
	a.action="sow"; sim.finish_farm(a)
	check(crop.stage==1 and a.seeds==1,"Harvested seeds replant the same ground")
	a.grain=8; a.pos=world.center(World.GRANARY); a.timer=3; a.action="deposit"
	sim.perform(a,0.2)
	check(sim.grain_store==6 and a.grain==2,"Farmer deposits actual grain in communal granary")
	var eater:Dictionary=sim.agents[0]
	eater.grain=0; eater.meat=0; eater.hunger=10; eater.pos=world.center(World.GRANARY); eater.timer=2; eater.action="fetch"
	sim.perform(eater,0.2)
	check(eater.grain==3 and sim.grain_store==3,"Hungry inhabitant withdraws real stored grain")
	eater.timer=4; eater.action="eat"; sim.perform(eater,0.2)
	check(eater.grain==2 and eater.hunger==52,"Eating grain consumes it and satisfies hunger")
	a.seeds=0; crop.stage=0; a.pos=world.center(crop.cell); a.goal=crop.cell; a.action="sow"
	sim.finish_farm(a)
	check(crop.stage==0,"No planting without seeds")
	var live:=Simulation.new(); live.setup(world)
	for i in 18000: live.tick(0.2)
	check(live.metrics.till>0 and live.crops.size()>world.initial_fields.size(),"Autonomous farmers open new plots beyond the original fields")
	var grain_total:int=live.grain_store
	var seed_total:=0
	var repeated:=false
	var valid_soil:=true
	for resident in live.agents:
		grain_total+=resident.grain; seed_total+=resident.seeds
	for field in live.crops:
		if field.cycles>1: repeated=true
		valid_soil = valid_soil and world.walkable(field.cell) and world.tile_at(field.cell)==World.LAND
	check(valid_soil,"Every crop occupies reachable soil, without covering roads or water")
	check(grain_total==live.metrics.harvest*3-live.metrics.grain_eaten,"Grain conservation across harvesting, storage, withdrawal and eating")
	check(seed_total==72+live.metrics.harvest*2-live.metrics.sow,"Seed conservation across every farming action")
	check(repeated,"At least one plot completes multiple autonomous harvest cycles")
	var save_path:="user://farming_verification_v2.json"
	live.save_state(save_path)
	var restored:=Simulation.new(); restored.setup(world)
	check(restored.load_state(save_path),"Farming save loads")
	check(restored.crops.size()==live.crops.size() and restored.grain_store==live.grain_store,"Expanded fields and communal grain persist")
	check(restored.crops[0].stage==live.crops[0].stage and restored.crops[0].cycles==live.crops[0].cycles,"Plant stages and harvest history persist")
	print("FARMING METRICS ",JSON.stringify(live.metrics))
	print("RESULT ",checks-failed,"/",checks," passed")
	world.queue_free(); quit(1 if failed else 0)
extends SceneTree
const World=preload("res://scripts/world.gd")
const Simulation=preload("res://scripts/simulation.gd")
var checks:=0
var failed:=0
func _initialize()->void:run.call_deferred()
func check(ok:bool,message:String)->void:
	checks+=1
	if ok:print("PASS: ",message)
	else:failed+=1;printerr("FAIL: ",message)
func run()->void:
	var world:=World.new();root.add_child(world);world.generate()
	var sim:=Simulation.new();sim.setup(world)
	for site in world.dungeon_sites:check(not world.path(world.center(World.HOME),site.cell).is_empty(),"Dungeon entrance reachable: "+site.name)
	for p in World.REST_SPOTS:check(world.walkable(p),"Inhabitant home is reachable")
	for animal in sim.animals:
		check((world.forest.has(world.cell(animal.pos)) if animal.kind==4 else world.pasture.has(world.cell(animal.pos))),"Wildlife and domestic livestock occupy their correct habitat")
	for a in sim.agents:a.hunger=90;a.thirst=90;a.energy=95
	var r=sim.realm
	check(r.request_expedition(0).contains("enviada"),"A click request recruits an eligible autonomous party")
	var d=r.dungeons[0]
	check(d.state=="approach" and d.party.size()==2,"Party must travel to the entrance first")
	for i in 1400:
		sim.advance(0.2)
		if d.state=="combat":break
	check(d.state=="combat","Real pathfinding reaches the first dungeon combat")
	check(sim.agents[d.party[0]].location=="dungeon","Dungeon occupants leave the overworld rendering")
	var save:="user://realm_verification_v3.json"
	check(sim.save_state(save)==OK,"Can save during combat")
	var world2:=World.new();root.add_child(world2);world2.generate()
	var resumed:=Simulation.new();resumed.setup(world2)
	check(resumed.load_state(save),"Can restore a live expedition")
	check(resumed.realm.dungeons[0].positions==d.positions and resumed.realm.dungeons[0].enemies==d.enemies,"Save preserves dungeon positions, enemy health and chest state")
	for i in 2000:
		resumed.advance(0.2)
		if resumed.metrics.expeditions>0:break
	check(resumed.metrics.dungeon_kills==3,"Autonomous party defeats all three enemies")
	check(resumed.metrics.artifacts_found==3,"Each guarded chest produces one actual artifact")
	check(resumed.metrics.expeditions==1,"Party walks back to the entrance and returns to the island")
	for i in 2500:
		resumed.advance(0.2)
		if resumed.metrics.sell_loot>=2:break
	check(resumed.metrics.sell_loot>0 and resumed.realm.treasury>60,"Recovered common artifacts sell at the market and fund the community")
	check(d.make_artifact(0.039,0).rare and not d.make_artifact(0.04,0).rare,"Rare artifact boundary is exactly four percent")
	var a:Dictionary=sim.agents[3]
	a.location="world";a.mission=-1;a.artifacts=[d.make_artifact(0.0,2)]
	var crop:Dictionary=sim.crops[0]
	crop.stage=3;a.pos=world.center(crop.cell);a.goal=crop.cell;a.action="harvest"
	var before:int=a.grain;sim.finish_farm(a)
	check(a.grain==before+4,"Rare harvest artifact adds an actual extra grain")
	a.artifacts=[d.make_artifact(0.0,3)];a.action="sleep";a.phase="act";a.energy=20;a.timer=0
	sim.perform(a,1)
	check(is_equal_approx(a.energy,21.625),"Rest artifact increases actual energy recovery by 25 percent")
	a.artifacts.clear()
	var soil:=Vector2i.ZERO
	for p in world.farm_sites:
		if sim.crop_at(p).is_empty() and p.distance_to(World.HOME)<10 and world.tile_at(p)==World.LAND:
			soil=p;break
	check(r.designate("farm",soil).contains("encargado"),"Player can designate a new plot outside authored fields")
	var job:Dictionary=r.jobs[-1];a.job=job.id;a.pos=world.center(job.goal);a.goal=job.goal;a.action="project";a.phase="act"
	r.complete_job(a)
	check(not sim.crop_at(soil).is_empty(),"A worker's completed task creates the new arable plot")
	r.designate("fallow",soil);job=r.jobs[-1];a.job=job.id;r.complete_job(a)
	check(sim.crop_at(soil).is_empty() and r.fallow.has(r.key(soil)),"Reclaimed grass is protected from immediate automatic replanting")
	var oldwood:int=r.wood;var oldcash:int=r.treasury
	r.designate("road",soil);job=r.jobs[-1];a.job=job.id;r.complete_job(a)
	check(world.tile_at(soil)==World.ROAD and r.wood==oldwood-1 and r.treasury==oldcash-1,"Road construction spends real resources and changes the terrain")
	var wood_before:int=r.wood;var herbs_before:int=r.herbs
	sim.grain_store=r.reserve+10
	check(r.dispatch(0).contains("viaje") and sim.grain_store==r.reserve,"Ship exports exactly the surplus while respecting reserves")
	r.tick(60)
	check(r.wood==wood_before+8 and r.herbs==herbs_before+2,"Ship returns with usable imported resources")
	var forest_cell:Vector2i=world.forest[0]
	r.designate("protect",forest_cell)
	check(not r.forest_options().has(forest_cell),"Forest protection removes gathering sites from autonomous work")
	a.goal=forest_cell;a.pos=world.center(forest_cell);a.action="gather";a.phase="act";a.timer=10
	oldwood=a.wood;sim.perform(a,1)
	check(a.wood==oldwood,"Already assigned gathering cannot harvest a newly protected grove")
	a.home_water=1;a.thirst=30;a.pos=world.center(a.home);a.action="drink_home";a.timer=0
	sim.perform(a,1)
	check(a.home_water<1 and a.thirst==39,"House water store actually depletes when used")
	r.livestock_food=3;a.action="tend";a.timer=5;a.eggs=0;sim.perform(a,0.2)
	check(a.eggs==3 and r.livestock_food==0,"Farm livestock produce collectable food, not decoration")
	a.action="eat";a.timer=4;a.hunger=20;sim.perform(a,0.2)
	check(a.eggs==2 and a.hunger==56,"Collected farm food can actually feed an inhabitant")
	check(sim.save_state(save)==OK and resumed.load_state(save),"Management changes save and load")
	check(resumed.world.tile_at(soil)==World.ROAD and resumed.realm.protected.has(r.key(forest_cell)),"Terrain construction and protected land persist")
	print("RESULT ",checks-failed,"/",checks," passed")
	quit(1 if failed else 0)

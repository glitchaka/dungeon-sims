extends RefCounted
const RARE_CHANCE := 0.04
const COLS:=35
const ROWS:=20
var id:=0
var name:=""
var level:=1
var entrance:=Vector2i.ZERO
var tiles:Dictionary={}
var nav:=AStarGrid2D.new()
var state:="idle"
var party:Array[int]=[]
var positions:Dictionary={}
var enemies:Array[Dictionary]=[]
var chests:Array[Dictionary]=[]
var room:=0
var combat_clock:=0.0
var cooldown_until:=0.0
var log:Array[String]=[]
var runs:=0
var kills:=0
var rooms:=[Rect2i(2,2,10,6),Rect2i(21,2,11,6),Rect2i(21,12,11,6),Rect2i(2,12,10,6)]
var centers:=[Vector2i(6,5),Vector2i(26,5),Vector2i(26,15),Vector2i(6,15)]

func _init(site:Dictionary,index:int)->void:
	id=index; name=site.name; level=site.level; entrance=site.cell
	for rect in rooms:
		for y in range(rect.position.y,rect.end.y):
			for x in range(rect.position.x,rect.end.x): tiles[Vector2i(x,y)]=true
	for i in range(1,centers.size()):
		var p:Vector2i=centers[i-1]
		while p!=centers[i]:
			tiles[p]=true; tiles[p+Vector2i.DOWN]=true
			if p.x!=centers[i].x: p.x+=signi(centers[i].x-p.x)
			else: p.y+=signi(centers[i].y-p.y)
	nav.region=Rect2i(0,0,COLS,ROWS); nav.cell_size=Vector2(16,16); nav.offset=Vector2(8,8)
	nav.diagonal_mode=AStarGrid2D.DIAGONAL_MODE_NEVER; nav.update()
	for y in ROWS:
		for x in COLS: nav.set_point_solid(Vector2i(x,y),not tiles.has(Vector2i(x,y)))
	reset_encounters()

func center(p:Vector2i)->Vector2:return Vector2(p*16)+Vector2(8,8)
func cell(p:Vector2)->Vector2i:return Vector2i(floori(p.x/16),floori(p.y/16))
func reset_encounters()->void:
	enemies.clear(); chests.clear(); room=0; combat_clock=0; positions.clear()
	for i in range(1,4):
		enemies.append({"room":i,"pos":center(centers[i]),"hp":22.0+level*7,"max_hp":22.0+level*7,"kind":"SAQUEADOR" if i%2 else "GUARDIAN"})
		chests.append({"room":i,"pos":center(centers[i]+Vector2i(3,-1)),"opened":false})
		nav.set_point_solid(centers[i],true)
		nav.set_point_solid(centers[i]+Vector2i(3,-1),true)
func note(message:String)->void:
	log.append(message)
	if log.size()>12: log.pop_front()

func start(sim,ids:Array[int])->void:
	reset_encounters(); party.assign(ids); state="approach"
	for who in party:
		var a:Dictionary=sim.agents[who]
		a.mission=id
		sim.begin(a,"expedition",entrance,"Viajando a "+name)
	note("La expedicion va de camino.")

func tick(sim,dt:float)->void:
	if state=="cooldown":
		if sim.elapsed>=cooldown_until: state="idle"
		return
	if state=="idle":return
	if state=="approach":
		var arrived:=true
		for who in party:
			var a:Dictionary=sim.agents[who]
			if a.pos.distance_to(sim.world.center(entrance))>6: arrived=false
		if arrived:
			state="exploring"; room=1
			for who in party:
				var a:Dictionary=sim.agents[who]
				a.location="dungeon"; a.phase="idle"; a.action="expedition"
				positions[str(who)]=center(centers[0])+Vector2(party.find(who)*6,0)
			note("Han entrado. Buscan la primera camara.")
		return
	var retreat:=false
	for who in party:
		var a:Dictionary=sim.agents[who]
		a.hunger=maxf(0,a.hunger-dt*0.03); a.thirst=maxf(0,a.thirst-dt*0.04); a.energy=maxf(0,a.energy-dt*0.06)
		a.frame+=dt*6
		if a.health<30 or a.thirst<14 or a.energy<12: retreat=true
	if retreat and state!="retreat":
		state="retreat"; note("Estan heridos o agotados: se retiran con el botin.")
	if state=="retreat" or state=="returning":
		var all_out:=true
		for who in party:
			move_hero(who,centers[0],dt)
			if positions[str(who)].distance_to(center(centers[0]))>8:all_out=false
		if all_out: finish(sim)
		return
	var enemy:Dictionary=enemies[room-1]
	var chest:Dictionary=chests[room-1]
	var ready:=true
	for who in party:
		var target:Vector2i=centers[room]+Vector2i(-2 if party.find(who)==0 else 2,0) if enemy.hp>0 else cell(chest.pos)+Vector2i(-1,party.find(who))
		var before:Vector2=positions[str(who)]
		move_hero(who,target,dt)
		var movement:Vector2=positions[str(who)]-before
		if movement.length()>0.01:sim.agents[who].direction=(1 if movement.x>0 else 3) if absf(movement.x)>absf(movement.y) else (0 if movement.y>0 else 2)
		if positions[str(who)].distance_to(center(target))>0.5:ready=false
	if enemy.hp<=0:state="looting"
	if not ready:return
	if enemy.hp>0:
		state="combat"; combat_clock+=dt
		for who in party:
			sim.agents[who].thought="Combatiendo: "+enemy.kind
			sim.agents[who].direction=1 if positions[str(who)].x<enemy.pos.x else 3
		if combat_clock>=1.0:
			combat_clock=0
			for who in party: enemy.hp=maxf(0,enemy.hp-sim.rng.randi_range(4,8))
			if enemy.hp>0:
				var target:int=party[sim.rng.randi_range(0,party.size()-1)]
				sim.agents[target].health=maxf(1,sim.agents[target].health-sim.rng.randi_range(2,4+level))
			else:
				nav.set_point_solid(cell(enemy.pos),false)
				kills+=1; sim.metrics.dungeon_kills+=1; note(enemy.kind+" derrotado.")
		return
	if not chest.opened:
		chest.opened=true
		var owner:Dictionary=sim.agents[party[(room-1)%party.size()]]
		var item:Dictionary=make_artifact(sim.rng.randf(),sim.rng.randi_range(0,3))
		if item.rare and not owner.artifacts.any(func(e:Dictionary)->bool:return e.bonus==item.bonus):
			owner.artifacts.append(item); sim.metrics.rare_artifacts+=1
			note(owner.name+" equipa "+item.name+".")
		else:
			owner.loot.append(item); note(owner.name+" encuentra "+item.name+".")
		sim.metrics.artifacts_found+=1
		sim.record(owner.name+" encontro "+item.name+" en "+name)
	if room>=3: state="returning";note("Mazmorra despejada. Regresan a vender.")
	else: room+=1;state="exploring"

func move_hero(who:int,target:Vector2i,dt:float)->void:
	var p:Vector2=positions[str(who)]
	var route:=nav.get_point_path(cell(p),target)
	if route.is_empty():return
	var dest:Vector2=route[1] if route.size()>1 else route[0]
	positions[str(who)]=p.move_toward(dest,dt*28)

func make_artifact(roll:float,variant:int)->Dictionary:
	if roll<RARE_CHANCE:
		var names:=["BOTAS DEL CAMINANTE","CANTIMPLORA ETERNA","HOZ DEL ALBA","AMULETO DEL REPOSO"]
		return {"name":names[variant],"value":90+level*20,"rare":true,"bonus":["speed","thirst","harvest","rest"][variant]}
	return {"name":["IDOLILLO ANTIGUO","CALIZ DE PLATA","SELLO DE BRONCE","GEMA DEL ECO"][variant],"value":12+level*8+variant*3,"rare":false,"bonus":""}

func finish(sim)->void:
	for who in party:
		var a:Dictionary=sim.agents[who]
		a.location="world";a.mission=-1;a.pos=sim.world.center(entrance);a.phase="idle";a.action="idle"
		a.thought="De vuelta de la mazmorra"
	state="cooldown";cooldown_until=sim.elapsed+300;runs+=1
	sim.metrics.expeditions+=1;sim.record("La expedicion de "+name+" regreso a la isla.")

func snapshot()->Dictionary:
	return {"state":state,"party":party,"positions":positions,"enemies":enemies,"chests":chests,"room":room,"combat_clock":combat_clock,"cooldown_until":cooldown_until,"log":log,"runs":runs,"kills":kills}
func restore(data:Dictionary)->void:
	state=data.state;party.assign(data.party);positions=data.positions
	enemies.assign(data.enemies);chests.assign(data.chests);room=int(data.room);combat_clock=data.combat_clock
	for enemy in enemies:
		enemy.room=int(enemy.room);nav.set_point_solid(cell(enemy.pos),enemy.hp>0)
	for chest in chests:chest.room=int(chest.room)
	cooldown_until=data.cooldown_until;log.assign(data.log);runs=int(data.runs);kills=int(data.kills)

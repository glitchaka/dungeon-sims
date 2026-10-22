extends RefCounted
const Dungeon = preload("res://scripts/dungeon_sim.gd")
const World = preload("res://scripts/world.gd")
var owner:WeakRef
var dungeons:Array=[]
var priority:="balanced"
var reserve:=30
var wood:=24
var treasury:=60
var herbs:=0
var trade_enabled:=false
var jobs:Array[Dictionary]=[]
var changes:Array[Dictionary]=[]
var protected:Dictionary={}
var fallow:Dictionary={}
var ships:Array[Dictionary]=[]
var next_job:=0
var last_dispatch:=0.0
var livestock_food:=6
var livestock_clock:=0.0

func _init(sim)->void:
	owner=weakref(sim)
	sim.metrics["tend"]=0
	for i in sim.world.dungeon_sites.size():dungeons.append(Dungeon.new(sim.world.dungeon_sites[i],i))
	for port in sim.world.ports:ships.append({"cell":port.cell,"remaining":0.0,"voyages":0})
func key(p:Vector2i)->String:return "%d,%d"%[p.x,p.y]
func tick(dt:float)->void:
	var sim=owner.get_ref()
	livestock_clock+=dt
	if livestock_clock>=50:
		livestock_clock=0;livestock_food=mini(12,livestock_food+2)
	for dungeon in dungeons:dungeon.tick(sim,dt)
	for ship in ships:
		if ship.remaining>0:
			ship.remaining=maxf(0,ship.remaining-dt)
			if ship.remaining==0:
				wood+=8;treasury+=15;herbs+=2;ship.voyages+=1;sim.metrics.trade+=1
				sim.record("El barco regreso: 8 madera, 15 monedas y 2 hierbas medicinales.")
	if trade_enabled and sim.elapsed-last_dispatch>90:dispatch(0)
	if priority=="explore" and int(sim.elapsed*5)%150==0:
		for dungeon in dungeons:
			if dungeon.state=="idle":request_expedition(dungeon.id);break
	for a in sim.agents:
		if a.location=="world" and a.health<70 and herbs>0 and a.pos.distance_to(sim.world.center(World.MARKET))<32:
			herbs-=1;a.health=minf(100,a.health+25);sim.record(a.name+" uso una hierba medicinal del puerto.")
	for job in jobs:
		if job.worker>=0:
			var a:Dictionary=sim.agents[int(job.worker)]
			if not a.alive or a.action!="project" or a.get("job",-1)!=job.id:job.worker=-1

func request_expedition(index:int)->String:
	var sim=owner.get_ref()
	var dungeon=dungeons[index]
	if dungeon.state!="idle":return "La expedicion ya esta en curso o la mazmorra se esta repoblando."
	var free_count:=0
	for a in sim.agents:
		if a.alive and a.mission<0:free_count+=1
	if free_count<4:return "Deben quedar al menos dos habitantes atendiendo la isla."
	var candidates:Array=[]
	for a in sim.agents:
		if a.alive and a.mission<0 and a.location=="world" and a.health>=65 and a.energy>=45 and a.hunger>30 and a.thirst>30:
			candidates.append(a)
	candidates.sort_custom(func(a:Dictionary,b:Dictionary)->bool:return (a.energy+ (40 if a.trait.begins_with("Explorador") else 0))>(b.energy+(40 if b.trait.begins_with("Explorador") else 0)))
	if candidates.size()<2:return "Se necesitan dos habitantes con salud 65, energia 45, comida y agua mayores de 30."
	var ids:Array[int]=[int(candidates[0].id),int(candidates[1].id)]
	dungeon.start(sim,ids)
	sim.record(candidates[0].name+" y "+candidates[1].name+" parten hacia "+dungeon.name)
	return "Expedicion enviada. Viajan por la isla antes de entrar."

func dispatch(index:int)->String:
	var sim=owner.get_ref()
	if ships[index].remaining>0:return "El barco sigue en alta mar."
	if sim.grain_store<reserve+10:return "Faltan 10 de grano por encima de la reserva de %d."%reserve
	sim.grain_store-=10;ships[index].remaining=60.0;last_dispatch=sim.elapsed
	sim.record("El puerto exporto 10 de grano; el barco busca recursos fuera de la isla.")
	return "Barco en viaje: regresara con madera, monedas y medicinas."

func designate(kind:String,p:Vector2i)->String:
	var sim=owner.get_ref()
	var map=sim.world
	if kind=="protect":
		if not map.forest.has(p):return "Elige un claro del bosque para proteger su entorno."
		var removing:bool=protected.has(key(p))
		for q in map.forest:
			if Vector2(q-p).length()<=5:
				if removing:protected.erase(key(q))
				else:protected[key(q)]=q
		return "Proteccion retirada." if removing else "Bosque protegido: aqui no se caza ni se recolecta madera."
	if not map.walkable(p) or map.tile_at(p)!=World.LAND or map.pasture.has(p) or protected.has(key(p)):
		return "Se necesita suelo libre y accesible; respeta el agua, los caminos y las reservas."
	if kind=="fallow" and sim.crop_at(p).is_empty():return "Elige un cultivo para devolverlo a pradera."
	if kind!="fallow" and not sim.crop_at(p).is_empty():return "Primero devuelve el cultivo a pradera."
	for job in jobs:
		if job.cell==p:return "Esa parcela ya tiene un trabajo pendiente."
	var cost_wood:=12 if kind=="house" else (1 if kind=="road" else 0)
	var cost_money:=15 if kind=="house" else (1 if kind=="road" else 0)
	if wood<cost_wood or treasury<cost_money:return "Recursos insuficientes: madera %d, monedas %d."%[cost_wood,cost_money]
	var goal:=p
	if kind=="house":
		var neighbors:Array[Vector2i]=[]
		for d in [Vector2i.LEFT,Vector2i.RIGHT,Vector2i.UP,Vector2i.DOWN]:
			if map.walkable(p+d):neighbors.append(p+d)
		if neighbors.size()<3:return "La casa bloquearia un paso estrecho."
		for a in sim.agents:
			if a.home==p or a.goal==p or map.cell(a.pos)==p:return "La parcela esta ocupada."
		goal=neighbors[0]
	wood-=cost_wood;treasury-=cost_money
	jobs.append({"id":next_job,"kind":kind,"cell":p,"goal":goal,"worker":-1});next_job+=1
	fallow.erase(key(p))
	return "Trabajo encargado. Un habitante lo realizara cuando cubra sus necesidades."

func choose_job(a:Dictionary)->bool:
	var sim=owner.get_ref()
	if sim.night() or a.energy<35 or a.hunger<40 or a.thirst<40:return false
	for job in jobs:
		if job.worker<0:
			job.worker=a.id;a.job=job.id
			sim.begin(a,"project",job.goal,{"farm":"Preparando cultivo encargado","fallow":"Recuperando pradera","house":"Construyendo vivienda","road":"Trazando un camino"}[job.kind])
			return true
	return false

func complete_job(a:Dictionary)->void:
	var sim=owner.get_ref()
	for job in jobs:
		if job.id!=a.get("job",-1):continue
		match job.kind:
			"farm":
				if sim.crop_at(job.cell).is_empty():sim.crops.append({"cell":job.cell,"stage":0,"growth":0.0,"ripe_age":0.0,"cycles":0})
			"fallow":
				sim.crops=sim.crops.filter(func(c:Dictionary)->bool:return c.cell!=job.cell)
				fallow[key(job.cell)]=true
			"house":
				# Keep every inhabitant outside the footprint before changing navigation.
				for person in sim.agents:
					if sim.world.cell(person.pos)==job.cell:person.pos=sim.world.center(job.goal)
				sim.world.make_house(job.cell);changes.append({"kind":"house","cell":job.cell})
				var resident:Dictionary=sim.agents[a.id]
				resident.home=job.goal;resident.home_town=-1
			"road":sim.world.set_road(job.cell);changes.append({"kind":"road","cell":job.cell})
		jobs.erase(job);sim.finish(a,"project","termino un encargo de "+job.kind);a.job=-1
		return
	a.phase="idle"

func forest_options()->Array[Vector2i]:
	var result:Array[Vector2i]=[]
	for p in owner.get_ref().world.forest:
		if not protected.has(key(p)):result.append(p)
	return result
func snapshot()->Dictionary:
	var ds:Array=[]
	for dungeon in dungeons:ds.append(dungeon.snapshot())
	return {"priority":priority,"reserve":reserve,"wood":wood,"treasury":treasury,"herbs":herbs,"trade_enabled":trade_enabled,"jobs":jobs,"changes":changes,"protected":protected,"fallow":fallow,"ships":ships,"next_job":next_job,"last_dispatch":last_dispatch,"dungeons":ds,"livestock_food":livestock_food,"livestock_clock":livestock_clock}
func restore(data:Dictionary)->void:
	priority=data.priority;reserve=int(data.reserve);wood=int(data.wood);treasury=int(data.treasury);herbs=int(data.herbs)
	livestock_food=int(data.get("livestock_food",0));livestock_clock=data.get("livestock_clock",0.0)
	trade_enabled=data.trade_enabled;jobs.assign(data.jobs);changes.assign(data.changes);protected=data.protected;fallow=data.fallow
	ships.assign(data.ships);next_job=int(data.next_job);last_dispatch=data.last_dispatch
	for change in changes:
		if change.kind=="house":owner.get_ref().world.make_house(change.cell)
		else:owner.get_ref().world.set_road(change.cell)
	for i in dungeons.size():dungeons[i].restore(data.dungeons[i])

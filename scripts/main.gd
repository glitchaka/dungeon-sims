extends Node2D
const World=preload("res://scripts/world.gd")
const Simulation=preload("res://scripts/simulation.gd")
const Actors=preload("res://scripts/actors.gd")
const Pixel=preload("res://scripts/pixel_text.gd")
const Local=preload("res://scripts/local_view.gd")
const BG=Color("121c28")
const MUTED=Color("8ba5b5")
const ACCENT=Color("ffcc88")
const NEED_NAMES=["COMIDA","AGUA","ENERGIA","SOCIAL","DIVERSION","HIGIENE"]
const NEED_HELP=["COMIDA: consume carne de la caza o grano cosechado.","AGUA: bebe en lagos y rios. El mar no sacia la sed.","ENERGIA: duerme en su casa o construye un refugio con madera.","SOCIAL: conversa con otros habitantes y mejora sus relaciones.","DIVERSION: explora el bosque de forma autonoma.","HIGIENE: se lava con agua dulce."]
var world:World
var sim:Simulation
var actors:Actors
var local_view:Local
var map_root:Node2D
var hud:Node2D
var background:Node2D
var selected:=0
var speed:=1.0
var paused:=false
var zoom:=1.0
var following:=false
var local_pan:=Vector2.ZERO
var local_zoom:=1.0
var pan:=Vector2.ZERO
var wall_time:=0.0
var autosave_timer:=0.0
var notice:="CLIC EN POBLADOS O MAZMORRAS PARA ENTRAR. GESTION PARA INFLUIR EN LA ISLA."
var save_path:="user://isla_viva_v3.json"
var screenshot_mode:=false
var hit_regions:Dictionary={}
var hover:=""
var inspected_crop:=Vector2i(-1,-1)
var journal:=false
var management:=false
var tool:="inspect"
var mode:="world"
var drag_window:=false
var drag_map:=false
var drag_origin:=Vector2i.ZERO
var window_origin:=Vector2i.ZERO
var size_ui:=Vector2(1120,960)
var map_area:=Rect2(0,86,1120,682)

func _ready()->void:
	texture_filter=CanvasItem.TEXTURE_FILTER_NEAREST
	get_window().borderless=true
	DisplayServer.window_set_title("Isla Viva")
	DisplayServer.set_icon(Actors.CHARACTERS.get_image().get_region(Rect2i(0,0,8,8)))
	get_tree().auto_accept_quit=false
	background=Node2D.new();add_child(background);background.draw.connect(draw_background)
	map_root=Node2D.new();add_child(map_root)
	world=World.new();map_root.add_child(world);world.generate()
	sim=Simulation.new();sim.setup(world)
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--capture"):screenshot_mode=true
	if not screenshot_mode:sim.load_state(save_path)
	actors=Actors.new();actors.sim=sim;map_root.add_child(actors)
	local_view=Local.new();local_view.sim=sim;add_child(local_view)
	hud=Node2D.new();hud.z_index=100;add_child(hud);hud.draw.connect(draw_hud)
	layout()
	if screenshot_mode:capture.call_deferred()

func layout()->void:
	size_ui=get_viewport_rect().size
	map_area=Rect2(0,86,size_ui.x,maxf(250,size_ui.y-278))
	var fit:=minf((map_area.size.x-24)/(World.W*8.0),(map_area.size.y-12)/(World.H*8.0))
	var scale_map:=fit*zoom
	map_root.scale=Vector2.ONE*scale_map
	map_root.position=map_area.get_center()-Vector2(World.W*4,World.H*4)*scale_map+pan
	if following and sim.agents[selected].location=="world":map_root.position=map_area.get_center()-sim.agents[selected].pos*scale_map
	map_root.visible=mode=="world"
	local_view.visible=mode!="world"
	var local_scale:=minf(map_area.size.x/560.0,map_area.size.y/336.0)
	local_view.scale=Vector2.ONE*local_scale
	local_view.position=map_area.get_center()-Vector2(280,168)*local_scale
	if mode=="town":
		local_scale=minf(map_area.size.x/640.0,map_area.size.y/400.0)*local_zoom
		local_view.scale=Vector2.ONE*local_scale
		local_view.position=map_area.get_center()-Vector2(320,254)*local_scale+local_pan

func _process(delta:float)->void:
	wall_time+=delta
	if not paused:sim.advance(minf(delta,0.25)*speed)
	actors.clock=wall_time;actors.selected=selected;actors.queue_redraw()
	world.modulate=Color(0.34,0.40,0.68).lerp(Color.WHITE,sim.daylight())
	local_view.selected=selected;local_view.clock=wall_time;local_view.queue_redraw()
	local_view.modulate=Color(0.52,0.57,0.76).lerp(Color.WHITE,sim.daylight()) if mode=="town" else Color.WHITE
	layout();background.queue_redraw();hud.queue_redraw()
	hover=""
	for id in hit_regions:
		if hit_regions[id].has_point(get_viewport().get_mouse_position()):hover=id
	autosave_timer+=delta
	if autosave_timer>45 and not screenshot_mode:
		autosave_timer=0
		if sim.save_state(save_path)!=OK:notice="NO SE PUDO GUARDAR LA PARTIDA."

func _unhandled_input(event:InputEvent)->void:
	if event is InputEventKey and event.pressed and not event.echo:
		match event.keycode:
			KEY_SPACE:activate("pause")
			KEY_1:activate("speed_1")
			KEY_2:activate("speed_4")
			KEY_3:activate("speed_12")
			KEY_4:activate("speed_32")
			KEY_TAB:selected=(selected+1)%sim.agents.size()
			KEY_F:activate("follow")
			KEY_G:activate("manage")
			KEY_F5:activate("save")
			KEY_F9:activate("load")
			KEY_J:activate("journal")
			KEY_ESCAPE:activate("back")
			KEY_F11:DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED if DisplayServer.window_get_mode()==DisplayServer.WINDOW_MODE_FULLSCREEN else DisplayServer.WINDOW_MODE_FULLSCREEN)
	if event is InputEventMouseMotion:
		if drag_window:DisplayServer.window_set_position(window_origin+DisplayServer.mouse_get_position()-drag_origin)
		if drag_map:
			if mode=="town":local_pan+=event.relative
			else:pan+=event.relative;following=false
	if event is InputEventMouseButton:
		if not event.pressed:
			if event.button_index==MOUSE_BUTTON_LEFT:drag_window=false
			if event.button_index in [MOUSE_BUTTON_RIGHT,MOUSE_BUTTON_MIDDLE]:drag_map=false
			return
		if event.button_index==MOUSE_BUTTON_WHEEL_UP:
			if mode=="town":local_zoom=minf(3,local_zoom*1.25)
			else:zoom=minf(6,zoom*1.25)
		elif event.button_index==MOUSE_BUTTON_WHEEL_DOWN:
			if mode=="town":local_zoom=maxf(0.65,local_zoom/1.25)
			else:zoom=maxf(1,zoom/1.25);pan=Vector2.ZERO if zoom==1 else pan
		elif event.button_index in [MOUSE_BUTTON_RIGHT,MOUSE_BUTTON_MIDDLE] and map_area.has_point(event.position):drag_map=true
		elif event.button_index==MOUSE_BUTTON_LEFT:
			# Last drawn controls take precedence over map labels underneath panels.
			var ids:Array=hit_regions.keys();ids.reverse()
			for id in ids:
				if hit_regions[id].has_point(event.position):activate(id);return
			if event.position.y<48 and DisplayServer.window_get_mode()==DisplayServer.WINDOW_MODE_WINDOWED:
				drag_window=true;drag_origin=DisplayServer.mouse_get_position();window_origin=DisplayServer.window_get_position();return
			if not map_area.has_point(event.position):return
			if journal or management:return
			if mode!="world":
				var id:=local_view.hit(local_view.to_local(event.position))
				if id.begins_with("house_") or id.begins_with("person_") or id=="back":activate(id)
				else:notice=local_view.inspect(id)
				return
			click_world(map_root.to_local(event.position))

func click_world(p:Vector2)->void:
	var cell:=world.cell(p)
	if tool!="inspect":notice=sim.realm.designate(tool,cell);return
	for i in world.dungeon_sites.size():
		if p.distance_to(world.center(world.dungeon_sites[i].cell))<15:open_dungeon(i);return
	var town:=world.settlement_at(cell)
	if town>=0:open_town(town);return
	for i in world.ports.size():
		if p.distance_to(world.center(world.ports[i].cell))<18:notice=sim.realm.dispatch(i);return
	for a in sim.agents:
		if a.location=="world" and p.distance_to(a.pos)<6:selected=a.id;notice=a.name+": "+a.thought;return
	var crop:=sim.crop_at(cell)
	if not crop.is_empty():inspected_crop=cell;return
	for animal in sim.animals:
		if animal.alive and p.distance_to(animal.pos)<6:
			notice=["GALLINA","GALLO","VACA","OVEJA","JABALI","CERDO"][animal.kind]+(": PRESA SILVESTRE; LA CAZA PRODUCE CARNE." if animal.kind==4 else ": GANADO DE LA ALDEA; VIVE EN EL PASTIZAL.");return
	for camp in sim.camps:
		if p.distance_to(world.center(camp.cell))<12:notice="REFUGIO DE "+sim.agents[camp.owner].name+": COSTO 3 DE MADERA Y PERMITE DORMIR FUERA DE LA ALDEA.";return
	for b in world.buildings:
		if b.title=="Casa nueva" and Rect2i(b.cell,b.size).has_point(cell):
			for a in sim.agents:
				if a.home_town<0 and Vector2(a.home-b.cell).length()<=1.5:activate("house_%d"%a.id);return

func open_town(index:int)->void:
	mode="town";local_pan=Vector2.ZERO;local_zoom=1.0;local_view.mode=mode;local_view.town=index;journal=false;management=false;inspected_crop=Vector2i(-1,-1)
	notice="CLIC EN LAS CASAS PARA VER SUS HOGARES. RUEDA: ZOOM. ARRASTRE DERECHO: RECORRER EL POBLADO."
func open_dungeon(index:int)->void:
	mode="dungeon";local_view.mode=mode;local_view.dungeon=index;journal=false;management=false;inspected_crop=Vector2i(-1,-1)
	notice=sim.realm.request_expedition(index) if sim.realm.dungeons[index].state=="idle" else "OBSERVANDO LA EXPEDICION. LAS NECESIDADES Y LAS HERIDAS PUEDEN OBLIGARLOS A RETIRARSE."

func activate(id:String)->void:
	if id.begins_with("person_"):selected=int(id.get_slice("_",1));return
	if id.begins_with("house_"):
		local_view.resident=int(id.get_slice("_",1));selected=local_view.resident;mode="house";local_view.mode=mode;return
	if id.begins_with("town_"):open_town(int(id.get_slice("_",1)));return
	if id.begins_with("dungeon_"):open_dungeon(int(id.get_slice("_",1)));return
	if id.begins_with("port_"):notice=sim.realm.dispatch(int(id.get_slice("_",1)));return
	if id.begins_with("need_"):notice=NEED_HELP[int(id.get_slice("_",1))];return
	if id.begins_with("speed_"):speed=float(id.get_slice("_",1));paused=false;return
	if id.begins_with("tool_"):tool=id.substr(5);management=false;mode="world";notice="MODO "+tool.to_upper()+": CLIC EN UNA PARCELA. ESC PARA CANCELAR.";return
	if id.begins_with("priority_"):sim.realm.priority=id.substr(9);return
	match id:
		"pause":paused=not paused
		"follow":following=not following;zoom=3 if following else 1;pan=Vector2.ZERO;mode="world"
		"save":notice="PARTIDA GUARDADA." if sim.save_state(save_path)==OK else "NO SE PUDO GUARDAR."
		"load":notice="PARTIDA RECUPERADA." if sim.load_state(save_path) else "NO HAY UN GUARDADO V3 COMPATIBLE."
		"journal":journal=not journal;management=false;inspected_crop=Vector2i(-1,-1)
		"manage":management=not management;journal=false;inspected_crop=Vector2i(-1,-1)
		"reserve":sim.realm.reserve=60 if sim.realm.reserve==30 else (10 if sim.realm.reserve==60 else 30)
		"trade":sim.realm.trade_enabled=not sim.realm.trade_enabled
		"send":notice=sim.realm.request_expedition(local_view.dungeon)
		"close_info":journal=false;management=false;inspected_crop=Vector2i(-1,-1)
		"back":
			if journal or management or inspected_crop.x>=0:activate("close_info")
			elif tool!="inspect":tool="inspect";notice="MODO OBSERVAR."
			elif mode=="house" and sim.agents[local_view.resident].home_town>=0:open_town(sim.agents[local_view.resident].home_town)
			else:mode="world"
		"exit":
			if sim.save_state(save_path)==OK:get_tree().quit()
			else:notice="NO SE PUDO GUARDAR. ALT-F4 CIERRA SIN GUARDAR."

func sprite(canvas:Node2D,texture:Texture2D,p:Vector2,source:Vector2i,size_px:=Vector2(8,8),tint:=Color.WHITE)->void:
	canvas.draw_texture_rect_region(texture,Rect2(p,size_px),Rect2(Vector2(source*8),Vector2(8,8)),tint)
func label_at(p:Vector2,value:String,size_px:int=2,color:Color=Color("fff1e8"))->void:Pixel.text(hud,p,value,size_px,color)
func panel(rect:Rect2)->void:Pixel.fill(hud,rect,Color("344454"));Pixel.fill(hud,rect.grow(-2),BG)
func button(id:String,rect:Rect2,title:String,active:bool=false)->void:
	hit_regions[id]=rect
	Pixel.fill(hud,rect,Color("e2ad65") if active else (Color("476176") if hover==id else Color("263a4b")))
	label_at(rect.position+Vector2(8,(rect.size.y-7)/2),title,1,Color("172130") if active else Color("fff1e8"))
func draw_background()->void:
	var tint:=Color(0.34,0.4,0.68).lerp(Color.WHITE,sim.daylight()) if sim else Color.WHITE
	if mode=="world":sprite(background,World.TILES,Vector2.ZERO,Vector2i(0,1),size_ui,tint)
	elif mode=="town":sprite(background,World.TILES,Vector2.ZERO,Vector2i(1,0),size_ui,Color("749b73"))
	else:Pixel.fill(background,Rect2(Vector2.ZERO,size_ui),Color("202a35"))

func draw_hud()->void:
	hit_regions.clear()
	if mode=="world":draw_map_labels()
	Pixel.fill(hud,Rect2(0,0,size_ui.x,86),BG)
	label_at(Vector2(16,11),"ISLA VIVA",2,ACCENT);label_at(Vector2(16,32),"ARRASTRA AQUI LA VENTANA",1,MUTED)
	label_at(Vector2(172,12),"DIA %d %02d:%02d"%[sim.day(),int(sim.hour()),int(fmod(sim.hour(),1)*60)],1)
	label_at(Vector2(172,30),"NOCHE" if sim.night() else "DIA",1,MUTED)
	button("pause",Rect2(294,8,68,30),"SEGUIR" if paused else "PAUSA",paused)
	for i in 4:
		var rate:int=[1,4,12,32][i];button("speed_%d"%rate,Rect2(368+i*40,8,36,30),"%dX"%rate,speed==rate and not paused)
	button("follow",Rect2(534,8,66,30),"ISLA" if following else "SEGUIR",following)
	button("manage",Rect2(606,8,78,30),"GESTION",management)
	button("journal",Rect2(690,8,68,30),"DIARIO",journal)
	button("save",Rect2(764,8,72,30),"GUARDAR")
	button("load",Rect2(842,8,64,30),"CARGAR")
	button("exit",Rect2(size_ui.x-78,8,64,30),"SALIR")
	button("back",Rect2(14,53,68,24),"VOLVER")
	var title:="CONTINENTE / "+("OBSERVAR" if tool=="inspect" else "ENCARGO: "+tool.to_upper())
	if mode=="town":title=world.settlements[local_view.town].name+" / POBLADO"
	elif mode=="house":title="HOGAR / CASA DE "+sim.agents[local_view.resident].name
	elif mode=="dungeon":title=sim.realm.dungeons[local_view.dungeon].name+" / "+{"idle":"DISPONIBLE","approach":"EN CAMINO","exploring":"EXPLORANDO","combat":"COMBATE","looting":"RECOGIENDO BOTIN","returning":"REGRESANDO","retreat":"RETIRADA","cooldown":"DESPEJADA"}[sim.realm.dungeons[local_view.dungeon].state]
	label_at(Vector2(97,61),title,1,ACCENT)
	label_at(Vector2(size_ui.x-384,61),"GRANO %d  MADERA %d  TESORO %d"%[sim.grain_store,sim.realm.wood,sim.realm.treasury],1,MUTED)
	draw_people()
	if mode=="dungeon":draw_expedition()
	if inspected_crop.x>=0:draw_crop_info()
	if journal:draw_journal()
	if management:draw_management()

func draw_map_labels()->void:
	for group in ["town","dungeon","port"]:
		var sites:Array=world.settlements if group=="town" else (world.dungeon_sites if group=="dungeon" else world.ports)
		for i in sites.size():
			var p:Vector2=map_root.to_global(world.center(sites[i].cell))+Vector2(9,-9)
			var rect:=Rect2(p,Vector2(sites[i].name.length()*6+16,22))
			if rect.position.y>88 and rect.end.y<map_area.end.y:button(group+"_%d"%i,rect,sites[i].name)

func draw_people()->void:
	var bottom:=size_ui.y-192
	Pixel.fill(hud,Rect2(0,bottom,size_ui.x,192),BG);Pixel.fill(hud,Rect2(0,bottom,size_ui.x,2),Color("56717e"))
	label_at(Vector2(18,bottom+15),"HABITANTES",1,MUTED)
	for a in sim.agents:
		var x:float=18+a.id*43
		hit_regions["person_%d"%a.id]=Rect2(x-3,bottom+30,38,48)
		if a.id==selected:Pixel.fill(hud,Rect2(x-3,bottom+30,38,48),Color("354e5b"))
		sprite(hud,Actors.sheet_for(a),Vector2(x+5,bottom+34),Vector2i.ZERO,Vector2(24,24))
		label_at(Vector2(x,bottom+66),a.name,1,ACCENT if a.id==selected else MUTED)
	var a:Dictionary=sim.agents[selected]
	label_at(Vector2(18,bottom+90),a.name+" / "+a.trait,1,ACCENT)
	label_at(Vector2(18,bottom+108),a.thought.left(43),1)
	label_at(Vector2(18,bottom+128),"MADERA %d CARNE %d GRANO %d"%[a.wood,a.meat,a.grain],1,MUTED)
	label_at(Vector2(18,bottom+144),"SEMILLAS %d MONEDAS %d SALUD %d"%[a.seeds,a.money,a.health],1,MUTED)
	label_at(Vector2(18,bottom+159),"BOTIN %d / BONUS %d"%[a.loot.size(),a.artifacts.size()],1,ACCENT)
	var col:float=(size_ui.x-330)/3.0
	for n in 6:
		var x:float=310+(n%3)*col;var y:float=bottom+16+floori(n/3.0)*67
		var value:float=a[Simulation.NEEDS[n]]
		hit_regions["need_%d"%n]=Rect2(x-8,y-6,col-8,56)
		label_at(Vector2(x,y),NEED_NAMES[n],2)
		label_at(Vector2(x+col-73,y+4),"%d/100"%value,1,MUTED)
		Pixel.fill(hud,Rect2(x,y+25,col-24,9),Color("293f4c"));Pixel.fill(hud,Rect2(x,y+25,(col-24)*value/100,9),Color("64b89c") if value>35 else Color("ef9562"))
	if not a.artifacts.is_empty():
		var item:Dictionary=a.artifacts[0]
		label_at(Vector2(310,bottom+157),"EQUIPADO: "+item.name+" / "+{"speed":"VELOCIDAD +15%","thirst":"SED -25%","harvest":"COSECHA +1","rest":"DESCANSO +25%"}[item.bonus],1,ACCENT)
	Pixel.fill(hud,Rect2(0,size_ui.y-21,size_ui.x,21),Color("20303d"))
	label_at(Vector2(18,size_ui.y-14),(NEED_HELP[int(hover.get_slice("_",1))] if hover.begins_with("need_") else notice).to_upper().left(int((size_ui.x-36)/6)),1,ACCENT)

func draw_expedition()->void:
	var d=sim.realm.dungeons[local_view.dungeon]
	panel(Rect2(16,98,340,98))
	label_at(Vector2(30,110),"NIVEL %d / EXPEDICIONES %d / ENEMIGOS %d"%[d.level,d.runs,d.kills],1,ACCENT)
	if d.state=="idle":button("send",Rect2(30,128,166,23),"ENVIAR 2 EXPLORADORES")
	else:
		for i in d.party.size():
			var a:Dictionary=sim.agents[d.party[i]]
			label_at(Vector2(30+i*154,135),"%s / SALUD %d"%[a.name,a.health],1)
		if d.state=="cooldown":label_at(Vector2(30,150),"NUEVA EXPEDICION EN %d S"%maxi(0,int(d.cooldown_until-sim.elapsed)),1,MUTED)
	label_at(Vector2(30,165),"BOTIN RARO: 4% POR COFRE. RETIRADA SI PELIGRAN.",1,MUTED)
	var y:float=map_area.end.y-67
	panel(Rect2(16,y,600,59))
	var first:int=maxi(0,d.log.size()-3)
	for i in range(first,d.log.size()):label_at(Vector2(28,y+10+(i-first)*15),d.log[i].left(92),1)

func draw_management()->void:
	panel(Rect2(20,100,690,368));label_at(Vector2(36,116),"GESTION DE LA ISLA",2,ACCENT)
	button("close_info",Rect2(666,108,30,24),"X")
	label_at(Vector2(36,148),"ENCARGOS: ELIGE UNA HERRAMIENTA Y LUEGO UNA PARCELA DEL MAPA.",1,MUTED)
	var kinds:=["inspect","farm","fallow","house","road","protect"]
	var names:=["OBSERVAR","CULTIVO","PRADERA","CASA","CAMINO","PROTEGER"]
	for i in kinds.size():button("tool_"+kinds[i],Rect2(36+i*107,169,101,28),names[i],tool==kinds[i])
	label_at(Vector2(36,211),"CASA: 12 MADERA + 15 MONEDAS. CAMINO: 1 MADERA + 1 MONEDA.",1)
	label_at(Vector2(36,230),"CULTIVO: UN TRABAJADOR PREPARA EL SUELO; EL AGRICULTOR SIEMBRA.",1,MUTED)
	label_at(Vector2(36,249),"PRADERA: RETIRA EL CULTIVO. PROTEGER: IMPIDE CAZA Y RECOLECCION.",1,MUTED)
	label_at(Vector2(36,279),"PRIORIDAD COMUNITARIA (RESPETA LAS NECESIDADES URGENTES)",1,ACCENT)
	for i in 4:
		var priorities:=["balanced","food","build","explore"]
		button("priority_"+priorities[i],Rect2(36+i*161,298,154,28),["EQUILIBRIO","ALIMENTACION","CONSTRUCCION","EXPEDICIONES"][i],sim.realm.priority==priorities[i])
	button("reserve",Rect2(36,343,190,28),"RESERVA: %d GRANO"%sim.realm.reserve)
	button("trade",Rect2(238,343,224,28),"COMERCIO AUTO: "+("SI" if sim.realm.trade_enabled else "NO"),sim.realm.trade_enabled)
	label_at(Vector2(36,388),"PUERTOS: 10 GRANO -> 8 MADERA + 15 MONEDAS + 2 HIERBAS.",1)
	label_at(Vector2(36,408),"LAS HIERBAS CURAN A LOS HERIDOS QUE REGRESAN AL MERCADO.",1,MUTED)
	label_at(Vector2(36,435),"ENCARGOS PENDIENTES %d / MEDICINAS %d"%[sim.realm.jobs.size(),sim.realm.herbs],1,ACCENT)

func draw_crop_info()->void:
	var crop:=sim.crop_at(inspected_crop)
	if crop.is_empty():return
	panel(Rect2(20,100,390,146));label_at(Vector2(36,116),"PARCELA DE TRIGO",2,ACCENT)
	button("close_info",Rect2(366,108,30,24),"X")
	label_at(Vector2(36,146),"ESTADO: "+["TIERRA PREPARADA","SEMBRADO","CRECIENDO","LISTO PARA COSECHAR"][int(crop.stage)],1)
	label_at(Vector2(36,166),"COSECHAS %d / CRECIMIENTO %d%%"%[crop.cycles,mini(100,int(crop.growth/120*100))],1)
	label_at(Vector2(36,186),"SIEMBRA: 1 SEMILLA. COSECHA: 3 GRANO + 2 SEMILLAS.",1,MUTED)
	label_at(Vector2(36,211),"GESTION > PRADERA DEVUELVE ESTA TIERRA AL PASTO.",1,ACCENT)
func draw_journal()->void:
	panel(Rect2(20,100,790,260));label_at(Vector2(36,116),"DIARIO DE LA ALDEA",2,ACCENT)
	button("close_info",Rect2(765,108,30,24),"X")
	var first:=maxi(0,sim.events.size()-10)
	for i in range(first,sim.events.size()):label_at(Vector2(36,148+(i-first)*19),sim.events[i].replace("|",":").left(122),1)

func _notification(what:int)->void:
	if what==NOTIFICATION_WM_CLOSE_REQUEST and sim!=null:sim.save_state(save_path);get_tree().quit()
func capture()->void:
	var args:=OS.get_cmdline_user_args()
	var name:="verification"
	if "--capture-night" in args:sim.advance(180);name+="-night"
	if "--capture-farming" in args:sim.advance(90);selected=3;name+="-farming"
	if "--capture-town" in args:open_town(0);name+="-town"
	if "--capture-house" in args:activate("house_0");name+="-house"
	if "--capture-dungeon" in args:
		for a in sim.agents:a.hunger=90;a.thirst=90;a.energy=95
		open_dungeon(0)
		for i in 1000:
			sim.advance(0.2)
			if sim.realm.dungeons[0].state=="combat":break
		name+="-dungeon"
	if "--capture-manage" in args:management=true;name+="-manage"
	paused=true;layout()
	await get_tree().process_frame
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png("res://"+name+".png")
	get_tree().quit()

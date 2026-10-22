extends SceneTree
var failed:=0
var checks:=0
func _initialize()->void: run.call_deferred()
func check(ok:bool,description:String)->void:
	checks+=1
	if ok: print("PASS: ",description)
	else: failed+=1; printerr("FAIL: ",description)
func click_at(p:Vector2)->void:
	var event:=InputEventMouseButton.new()
	event.button_index=MOUSE_BUTTON_LEFT; event.position=p; event.global_position=p; event.pressed=true
	root.push_input(event)
	await process_frame
	event=InputEventMouseButton.new()
	event.button_index=MOUSE_BUTTON_LEFT; event.position=p; event.global_position=p; event.pressed=false
	root.push_input(event)
	await process_frame
func click(game:Node,id:String)->void:
	await click_at(game.hit_regions[id].get_center())
func run()->void:
	var game=load("res://main.tscn").instantiate()
	game.save_path="user://ui_verification_v3.json"
	if FileAccess.file_exists(game.save_path):DirAccess.remove_absolute(game.save_path)
	root.add_child(game)
	game.set_process(false); game.screenshot_mode=true
	await process_frame
	await process_frame
	check(game.get_window().borderless,"Window has no native title bar or frame")
	check(game.hit_regions.has_all(["pause","save","load","follow","journal","exit"]),"Visible control buttons have interactive hit areas")
	await click(game,"pause")
	var before:float=game.sim.elapsed
	game._process(0.1)
	check(game.paused and game.sim.elapsed==before,"Mouse pause button actually pauses time")
	await click(game,"speed_12")
	game._process(0.1)
	check(game.speed==12 and not game.paused and game.sim.elapsed>before+1,"Speed button resumes and accelerates simulation")
	await click(game,"speed_1")
	check(game.speed==1,"Mouse speed button restores normal time")
	await click(game,"person_4")
	check(game.selected==4,"Portrait click changes inspected inhabitant")
	await click(game,"need_0")
	check(game.notice.contains("consume carne"),"Need bar click explains its actual function")
	await click(game,"follow"); game._process(0)
	check(game.zoom==3 and game.following and game.map_root.to_global(game.sim.agents[game.selected].pos).distance_to(game.map_area.get_center())<0.01,"Follow button follows selected inhabitant")
	await click(game,"follow"); game._process(0)
	check(game.zoom==1 and not game.following,"Overview button restores authored island")
	game.hud.queue_redraw();await process_frame;await process_frame
	await click(game,"town_0");game._process(0)
	game.local_view.queue_redraw();await process_frame;await process_frame
	check(game.mode=="town","Clicking a settlement opens its detailed village")
	await click_at(game.local_view.to_global(game.local_view.regions["house_0"].get_center()))
	check(game.mode=="house" and game.local_view.resident==0,"Clicking a village house opens its resident's interior")
	game.local_view.queue_redraw();await process_frame;await process_frame
	await click_at(game.local_view.to_global(game.local_view.regions["pantry"].get_center()))
	check(game.notice.contains("PROVISIONES DE"),"Interior furniture reveals real resident inventories")
	game.activate("back");check(game.mode=="town","Back returns from house to village")
	game.activate("back");game._process(0);game.hud.queue_redraw();await process_frame;await process_frame
	for person in game.sim.agents:person.hunger=90;person.thirst=90;person.energy=95
	await click(game,"dungeon_0");game._process(0)
	check(game.mode=="dungeon" and game.sim.realm.dungeons[0].state=="approach","Clicking dungeon opens view and dispatches a real expedition")
	game.activate("back");game._process(0);game.hud.queue_redraw();await process_frame;await process_frame
	await click(game,"manage");game.hud.queue_redraw();await process_frame;await process_frame
	await click(game,"priority_food")
	check(game.sim.realm.priority=="food","Community priority button updates autonomous farming policy")
	await click(game,"reserve")
	check(game.sim.realm.reserve==60,"Reserve control protects communal food from export")
	await click(game,"tool_farm")
	check(game.tool=="farm" and not game.management,"Farm tool arms a designation, not a movement command")
	game.activate("back")
	var crop:Dictionary=game.sim.crops[0]
	await click_at(game.map_root.to_global(game.world.center(crop.cell)))
	check(game.inspected_crop==crop.cell,"Clicking wheat opens its live crop inspector")
	game.hud.queue_redraw(); await process_frame; await process_frame
	check(game.hit_regions.has("close_info"),"Crop inspector includes a working close button")
	await click(game,"close_info")
	check(game.inspected_crop.x==-1,"Crop inspector closes through its button")
	await click(game,"journal")
	check(game.journal,"Journal button opens real simulation events")
	await click(game,"journal")
	var a:Dictionary=game.sim.agents[game.selected]
	var goal:Vector2i=a.goal
	await click_at(Vector2(50,400))
	check(a.goal==goal,"Clicking empty map never issues a movement order")
	a.grain=7
	await click(game,"save")
	a.grain=0
	await click(game,"load")
	check(game.sim.agents[game.selected].grain==7,"Save and load buttons restore actual inventory")
	for direction in 4:
		var frames:Dictionary={}
		for step in 4:
			var frame:Vector2i=game.actors.frame_cell({"phase":"move","frame":step,"direction":direction})
			frames[frame]=true
		check(frames.size()==4,"Four atlas animation frames for direction "+str(direction))
	check(game.actors.sheet_for({"id":0}).resource_path.contains("MAS-Characters-P8") and game.actors.sheet_for({"id":1}).resource_path.contains("MAS-Characters-DB"),"Residents use original human sprites from both supplied palettes")
	game.hud.queue_redraw(); await process_frame
	if DisplayServer.get_name()!="headless":
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://verification-ui.png")
	if DisplayServer.get_name()!="headless":
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
		await process_frame;await process_frame;game._process(0)
		check(game.size_ui.x>1120 and game.map_area.size.x==game.size_ui.x,"Fullscreen expands the canvas to fill the screen without letterboxing")
		await process_frame;await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://verification-fullscreen.png")
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
		await process_frame
	print("RESULT ",checks-failed,"/",checks," passed")
	game.queue_free(); quit(1 if failed else 0)
extends RefCounted
## Tagged vectors preserve nested expedition state without relying on JSON string coercion.
static func encode(value):
	if value is Vector2i:return {"@cell":[value.x,value.y]}
	if value is Vector2:return {"@point":[value.x,value.y]}
	if value is Dictionary:
		var out:Dictionary={}
		for key in value:out[str(key)]=encode(value[key])
		return out
	if value is Array or value is PackedVector2Array:
		var out:Array=[]
		for item in value:out.append(encode(item))
		return out
	return value
static func decode(value):
	if value is Dictionary:
		if value.has("@cell"):return Vector2i(value["@cell"][0],value["@cell"][1])
		if value.has("@point"):return Vector2(value["@point"][0],value["@point"][1])
		var out:Dictionary={}
		for key in value:out[key]=decode(value[key])
		return out
	if value is Array:
		var out:Array=[]
		for item in value:out.append(decode(item))
		return out
	return value
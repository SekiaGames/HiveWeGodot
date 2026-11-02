shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_disabled, diffuse_lambert, specular_disabled;

uniform sampler2DArray ground_tex : source_color, filter_linear, repeat_enable;
uniform sampler2D height_map : filter_linear, repeat_disable;
uniform vec2 vertex_size = vec2(193.0, 97.0);
uniform sampler2D pathing_overlay : filter_nearest, repeat_disable;
uniform bool show_pathing = false;

varying vec4 texture_layers;
varying vec2 world_uv;

void vertex() {
	vec2 tile = MODEL_MATRIX[3].xz;
	vec2 map_uv = (tile + UV) / max(vertex_size - vec2(1.0), vec2(1.0));
	VERTEX.y = texture(height_map, map_uv).r;
	texture_layers = INSTANCE_CUSTOM;
	world_uv = map_uv;
}

void fragment() {
	vec4 color = texture(ground_tex, vec3(UV, texture_layers.w));
	color = mix(texture(ground_tex, vec3(UV, texture_layers.z)), color, color.a);
	color = mix(texture(ground_tex, vec3(UV, texture_layers.y)), color, color.a);
	color = mix(texture(ground_tex, vec3(UV, texture_layers.x)), color, color.a);

	if (show_pathing) {
		vec4 overlay = texture(pathing_overlay, world_uv);
		color.rgb = mix(color.rgb, overlay.rgb, overlay.a);
	}

	ALBEDO = color.rgb;
}

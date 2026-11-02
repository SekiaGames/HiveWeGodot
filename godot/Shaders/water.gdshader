shader_type spatial;
render_mode blend_mix, cull_disabled, depth_draw_opaque, unshaded, shadows_disabled;

uniform vec4 shallow_color : source_color = vec4(0.15, 0.45, 0.55, 0.55);
uniform vec4 deep_color : source_color = vec4(0.05, 0.18, 0.30, 0.80);
uniform float wave_strength = 0.06;
uniform float wave_speed = 1.2;

void vertex() {
	vec4 corners = INSTANCE_CUSTOM;
	float top = mix(corners.x, corners.y, UV.x);
	float bottom = mix(corners.z, corners.w, UV.x);
	float wave = sin(TIME * wave_speed + (MODEL_MATRIX[3][0] + MODEL_MATRIX[3][2]) * 0.35) * wave_strength;
	VERTEX.y = mix(top, bottom, UV.y) + wave;
}

void fragment() {
	ALBEDO = mix(deep_color.rgb, shallow_color.rgb, 0.45);
	ALPHA = mix(deep_color.a, shallow_color.a, 0.6);
}

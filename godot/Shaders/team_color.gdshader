shader_type spatial;
render_mode cull_disabled, depth_draw_opaque, specular_disabled;

uniform vec4 team_color : source_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform float layer_alpha = 1.0;
uniform sampler2D mask_texture : source_color, filter_linear_mipmap, repeat_enable;

void fragment() {
	ALBEDO = team_color.rgb * texture(mask_texture, UV).r;
	ALPHA = layer_alpha;
	ROUGHNESS = 0.85;
	METALLIC = 0.0;
}

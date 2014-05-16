camera_archetype = {
	transform = {
		pos = vec2(),
		rotation = 0
	},

	camera = {
		enabled = true,
		
		layer = 0, -- 0 = topmost
		mask = render_component.WORLD,
		
		enable_smoothing = true,
		smoothing_average_factor = 0.5,
		averages_per_sec = 20,
		
		crosshair = nil,
		player = nil,
	
		orbit_mode = camera_component.LOOK,
		max_look_expand = vec2(config_table.resolution_w/2, config_table.resolution_h/2),
		angled_look_length = 100
	},
	
	chase = {
		relative = false
		--offset = vec2(config_table.resolution_w/(-2), config_table.resolution_h/(-2))
	}
}
 
scene_fbo = framebuffer_object(config_table.resolution_w, config_table.resolution_h)

film_grain_vertex_shader = GLSL_shader(GL.GL_VERTEX_SHADER, [[
#version 330

layout(location = 0) in vec2 position;

//smooth out vec2 theTexcoord;

void main() 
{
	gl_Position = (vec4(position.xy, 0, 1) * 2.0) - 1.0;
	//theTexcoord = position;
}

]])

film_grain_fragment_shader = GLSL_shader(GL.GL_FRAGMENT_SHADER, [[
#version 330
uniform float time;

// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}



// Pseudo-random value in half-open range [0:1].
float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }

out vec4 outputColor;

void main() 
{
	vec3  inputs = vec3( gl_FragCoord.xy, time ); // Spatial and temporal inputs
    float rand   = random( inputs );              // Random per-pixel value
    vec3  luma   = vec3( rand );                  // Expand to RGB
	
    outputColor = vec4(luma, 0.1);
}

]])

my_vertex_shader = GLSL_shader(GL.GL_VERTEX_SHADER, [[
#version 330

uniform mat4 projection_matrix;
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec4 color;

smooth out vec4 theColor;
 out vec2 theTexcoord;

void main() 
{
	vec4 output_vert;
	output_vert.x = position.x;		
	output_vert.y = position.y;				
	output_vert.z = 0.0f;						
	output_vert.w = 1.0f;
	
	gl_Position = projection_matrix*output_vert;
	theColor = color;
	theTexcoord = texcoord;
}

]])
 
my_fragment_shader = GLSL_shader(GL.GL_FRAGMENT_SHADER, [[
#version 330
smooth in vec4 theColor;
 in vec2 theTexcoord;

out vec4 outputColor;

uniform sampler2D basic_texture;

void main() 
{
    outputColor = theColor * texture(basic_texture, theTexcoord);
}

]])

film_grain_program = GLSL_program()
film_grain_program:attach(film_grain_vertex_shader)
film_grain_program:attach(film_grain_fragment_shader)
film_grain_program:use()
time_uniform = GL.glGetUniformLocation(film_grain_program.id, "time")

my_shader_program = GLSL_program()
my_shader_program:attach(my_vertex_shader)
my_shader_program:attach(my_fragment_shader)
my_shader_program:use()

projection_matrix_uniform = GL.glGetUniformLocation(my_shader_program.id, "projection_matrix")
basic_texture_uniform = GL.glGetUniformLocation(my_shader_program.id, "basic_texture")

GL.glUniform1i(basic_texture_uniform, 0)

local my_timer = timer()

world_camera_ptr = ptr_create_entity (archetyped(camera_archetype, {
	transform = {
		pos = vec2(),
		rotation = 0
	},

	camera = {
		screen_rect = rect_xywh(0, 0, config_table.resolution_w, config_table.resolution_h),
		size = vec2(config_table.resolution_w, config_table.resolution_h),
		
		drawing_callback = function (subject, camera_draw_input, mask)
			local renderer = camera_draw_input.output
			local visible_area = camera_draw_input.visible_area
			
			my_shader_program:use()
			renderer:generate_triangles(camera_draw_input, mask)
			
			GL.glUniformMatrix4fv(
			projection_matrix_uniform, 
			1, 
			GL.GL_FALSE, 
			orthographic_projection(0, visible_area.x, visible_area.y, 0, 0, 1):data()
			)
			
			--local old_num = renderer:get_triangle_count()
			--
			--local drawn_dist = (drawn_transform.pos - target_transform.pos):length()
			--if drawn_dist > 20 then
			--	target_transform.pos = target_transform.pos + vec2(randval(-drawn_dist/10, drawn_dist/10), randval(-drawn_dist/10, drawn_dist/10))
			--	renderer:generate_triangles(visible_area, target_transform, mask)
			--	
			--	for i=old_num, renderer:get_triangle_count()-1 do
			--		local my_tri = renderer:get_triangle(i)
			--		
			--		my_tri:get_vert(0).color.a = my_tri:get_vert(0).color.a * (drawn_dist / 300)
			--		my_tri:get_vert(1).color.a = my_tri:get_vert(1).color.a * (drawn_dist / 300)
			--		my_tri:get_vert(2).color.a = my_tri:get_vert(2).color.a * (drawn_dist / 300)
			--		
			--		my_tri:get_vert(0).color.r = my_tri:get_vert(0).color.r * (drawn_dist / 100)
			--		my_tri:get_vert(1).color.r = my_tri:get_vert(1).color.r * (drawn_dist / 100)
			--		my_tri:get_vert(2).color.r = my_tri:get_vert(2).color.r * (drawn_dist / 100)
			--		
			--		my_tri:get_vert(0).color.g = my_tri:get_vert(0).color.g * (drawn_dist / 200)
			--		my_tri:get_vert(1).color.g = my_tri:get_vert(1).color.g * (drawn_dist / 200)
			--		my_tri:get_vert(2).color.g = my_tri:get_vert(2).color.g * (drawn_dist / 200)
			--		
			--		my_tri:get_vert(0).color.b = my_tri:get_vert(0).color.b * (drawn_dist / 150)
			--		my_tri:get_vert(1).color.b = my_tri:get_vert(1).color.b * (drawn_dist / 150)
			--		my_tri:get_vert(2).color.b = my_tri:get_vert(2).color.b * (drawn_dist / 150)
			--	end
			--end
			--
			--scene_fbo:use()
			--
			--GL.glClear(GL.GL_COLOR_BUFFER_BIT)
			--
			--renderer:call_triangles()
			--
			--framebuffer_object.use_default()
			--
			--GL.glColor4f(1, 1, 1, 1)
			--GL.glBindTexture(GL.GL_TEXTURE_2D, scene_fbo:get_texture_id())
			--
			--GL.glGenerateMipmap(GL.GL_TEXTURE_2D)
			--
			--GL.glBegin(GL.GL_QUADS)
			--
			--GL.glTexCoord2f(0, 1); GL.glVertex2i(visible_area.x, visible_area.y)
			--GL.glTexCoord2f(1, 1); GL.glVertex2i(visible_area.r, visible_area.y)
			--GL.glTexCoord2f(1, 0); GL.glVertex2i(visible_area.r, visible_area.b)
			--GL.glTexCoord2f(0, 0); GL.glVertex2i(visible_area.x, visible_area.b)
			--
			--GL.glEnd()
			--
			renderer:default_render(visible_area)
			renderer:clear_triangles()
			
			film_grain_program:use()
			GL.glUniform1f(time_uniform, my_timer:get_milliseconds())
			
			GL.glBegin(GL.GL_QUADS)	
				GL.glVertexAttrib2f(0,1,1);
				GL.glVertexAttrib2f(0,1,0);
				GL.glVertexAttrib2f(0,0,0);
				GL.glVertexAttrib2f(0,0,1);
			GL.glEnd()
		end
	},
	
	input = {
		intent_message.SWITCH_LOOK,
		custom_intents.ZOOM_CAMERA
	},
	
	chase = {},
	
	scriptable = {
		script_data = {}
	}
}))

-- convenience, will always exist
world_camera = world_camera_ptr:get()

world_camera_self = generate_entity_object(world_camera_ptr, camera_class)

world_camera_self.intent_message = function(self, message)
	if message.intent == custom_intents.ZOOM_CAMERA then
		self:set_zoom_level(current_zoom_level-message.wheel_amount)
	end
	return false
end


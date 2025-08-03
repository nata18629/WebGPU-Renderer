struct VertexInput {
    @location(0) position: vec2f,
    @location(1) coord: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec3f,
};

@group(0) @binding(0) var imageTexture: texture_2d<f32>;
@group(0) @binding(1) var<uniform> uTime: f32;

@vertex
fn vs_main(@location(0) vertex_pos: vec3f) -> VertexOutput {
    //let coords = (vertex_pos+1.0)*100.0;
    let ratio = 640.0 / 480.0;
    var offset = vec2f(-0.6875, -0.463);
    offset += 0.3 * vec2f(cos(uTime), sin(uTime));
    let angle = uTime; // you can multiply it go rotate faster
    let alpha = cos(angle);
    let beta = sin(angle);

    var pos = vec4f(vertex_pos.x+offset.x,
    (vertex_pos.y+offset.y)*alpha,
    alpha*vertex_pos.z- beta*(vertex_pos.y),
    1.0);
    pos.y*=ratio;
    pos.z=pos.z*0.5+0.5;
    //let uv = -vertex_pos*0.6+vec3f(0.9,0.8,0.0);
    let uv = -vertex_pos*0.8+vec3f(1.0,0.9,0.0);

    return VertexOutput(pos, uv);
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let texelCoords = vec2i(in.uv.xy * vec2f(textureDimensions(imageTexture)));
    let color = textureLoad(imageTexture, texelCoords, 0).rgb;
return vec4f(color,1.0);
}

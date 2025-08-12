struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f
};

@group(0) @binding(0) var imageTexture: texture_2d<f32>;
@group(0) @binding(1) var<uniform> uTime: f32;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    //let coords = (position+1.0)*100.0;
    let objectTranslate = vec3f(0.0,0.0,0.0);

    let ratio = 640.0 / 480.0;
    var offset = vec2f(-0.6875, -0.463);
    offset += 0.3 * vec2f(cos(uTime), sin(uTime));
    let angle = uTime;
    let cos1 = cos(angle);
    let sin1 = sin(angle);

    let rotYZ = transpose(mat3x3f(
        1.0, 0.0, 0.0,
        0.0, cos1, sin1,
        0.0, -sin1, cos1,
    ));

    let rotXY = transpose(mat3x3f(
        cos1, sin1, 0.0,
        -sin1, cos1, 0.0,
        0.0, 0.0, 1.0,
    ));

    var pos = vec3f(in.position);
    pos = rotXY*rotYZ*pos;

    // let P = transpose(mat4x4f(
    //     1.0, 0.0, 0.0, 0.0,
    //     0.0, ratio, 0.0, 0.0,
    //     0.0, 0.0, 0.5, 0.5,
    //     0.0, 0.0, 0.0, 1.0,
    // ));
    let focalPoint = vec3f(0.0, 0.0, -2.0);
    pos = pos - focalPoint;

    // We divide by the Z coord
    let focalLength = 0.8;
    pos.x /= pos.z/focalLength;
    pos.y /= pos.z/focalLength;

    let near = 0.01;
    let far = 100.0;
    let scale = 1.0;
    let P = transpose(mat4x4f(
        1.0 / scale,      0.0,           0.0,                  0.0,
            0.0,     ratio / scale,      0.0,                  0.0,
            0.0,          0.0,      1.0 / (far - near), -near / (far - near),
            0.0,          0.0,           0.0,                  1.0,
    ));
    
    // let a = pos.z+2.0;
    // pos.x/=a;
    // pos.y/=a;

    let uv = -in.position.xyz*0.6+vec3f(0.9,0.8,0.0);
    //let uv = -position*0.8+vec3f(1.0,0.9,0.0);
    return VertexOutput(P*vec4f(pos+objectTranslate,1.0),
    uv, in.normal, in.color);
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    //let texelCoords = vec2i(in.uv.xy * vec2f(textureDimensions(imageTexture)));
    //let color = textureLoad(imageTexture, texelCoords, 0).rgb;
    let lightDirection1 = vec3f(0.5, -0.9, 0.1);
    let lightDirection2 = vec3f(0.2, 0.4, 0.3);
    let shading1 = max(0.0, dot(lightDirection1, in.normal));
    let shading2 = max(0.0, dot(lightDirection2, in.normal));
    let shading = shading1 + shading2;
    let color = in.color * shading;
    return vec4f(color,1.0);
}

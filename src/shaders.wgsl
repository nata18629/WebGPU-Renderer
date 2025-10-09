struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f,
    @location(3) uv: vec2f
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f,
    @location(3) viewDirection: vec3f
};

struct Uniforms {
    view: mat4x4f,
    cameraPos: vec3f,
    time: f32
}

struct ObjectTransforms {
    rot: mat4x4f,
    scale: mat4x4f,
    trans: mat4x4f
}

@group(0) @binding(0) var<uniform> uUniforms: Uniforms;
@group(0) @binding(1) var textureSampler: sampler;
@group(1) @binding(0) var imageTexture: texture_2d<f32>;
@group(1) @binding(1) var<uniform> uObjTrans: ObjectTransforms;
@group(1) @binding(2) var normalTexture: texture_2d<f32>;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    //let coords = (position+1.0)*100.0;
    let objectTranslate = vec3f(0.0,0.0,0.0);

    let ratio = 640.0 / 480.0;
    var offset = vec2f(-0.6875, -0.463);
    offset += 0.3 * vec2f(cos(uUniforms.time), sin(uUniforms.time));
    let angle = uUniforms.time;
    let cos1 = cos(angle);
    let sin1 = sin(angle);

    let model = uObjTrans.rot;
    
    let focalPoint = vec3f(0.0, 0.0, -2.0);
    let viewT = transpose(mat4x4f(
    1.0,  0.0, 0.0, -focalPoint.x,
    0.0,  1.0, 0.0, -focalPoint.y,
    0.0,  0.0, 1.0, -focalPoint.z,
    0.0,  0.0, 0.0,     1.0
    ));

    let focalLength = 1.2;

    let near = 0.01;
    let far = 100.0;
    let scale = 1.0;
    let divider = 1 / (focalLength * (far - near));
    let P = transpose(mat4x4f(
        1.0 / scale,      0.0,           0.0,                  0.0,
            0.0,     ratio / scale,      0.0,                  0.0,
            0.0,          0.0,      far*divider, -far*near*divider,
            0.0,          0.0,           1.0/focalLength,                  0.0,
    ));
    
    let worldPosition = model * vec4f(in.position, 1.0);
    let view = uUniforms.cameraPos - worldPosition.xyz;
    let position = P*viewT*uUniforms.view*worldPosition;
    let uv = in.uv;
    return VertexOutput(position,
    uv, in.normal, in.color, view);
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    //let texCoords = vec2i(in.uv * vec2f(textureDimensions(imageTexture)));
    var color = textureSample(imageTexture, textureSampler, in.uv).rgb;
    let lightDirection1 = vec3f(0.5, -0.5, 0.1);
    let lightDirection2 = vec3f(0.2, 0.4, 0.3);
    let L = vec3f(0.9, -0.9, 0.1);
    
    let diffuse = max(0.0, dot(L, in.normal));

    var specular = 0.0;
    
    //let N = in.normal;
    let encodedN = textureSample(normalTexture, textureSampler, in.uv).rgb;
    let N = normalize(encodedN - 0.5);
    let R = reflect(-L, N);
    let V = normalize(in.viewDirection);
    let RoV = max(0.0, dot(R, V));
    let hardness = 3.0;
    specular = pow(RoV, hardness);
    let ambient = 0.05;
    color *= specular+diffuse+ambient;

    return vec4f(color,1.0);
}

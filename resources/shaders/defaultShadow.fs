#version 460 core
out vec4 FragColor;

#define MAX_NR_SHADOWS 4

in VS_OUT {
    vec4 FragPosLightSpace[MAX_NR_SHADOWS];
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

struct Material {
    sampler2D diffuse;
    sampler2D specular;    
    float shininess;
}; 

struct DirLight {
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool hasShadow;
    int shadowIndex;

    sampler2D shadowMap;
};  

struct PointLight {    
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool hasShadow;
    int shadowIndex;

    float constant;
    float linear;
    float quadratic;  

    float farPlane;

    samplerCube shadowMap;
}; 

struct SpotLight {
    vec3 position;
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
    
    bool hasShadow;
    int shadowIndex;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    sampler2D shadowMap;
};
  
uniform vec3 viewPos;
uniform Material material;

#define MAX_NR_DIR_LIGHTS 2
uniform int numDirLights;
uniform DirLight dirLights[MAX_NR_DIR_LIGHTS];

#define MAX_NR_POINT_LIGHTS 4  
uniform int numPointLights;
uniform PointLight pointLights[MAX_NR_POINT_LIGHTS];

#define MAX_NR_SPOT_LIGHTS 4
uniform int numSpotLights;
uniform SpotLight spotLights[MAX_NR_SPOT_LIGHTS];

uniform bool blinn;

// function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float CalcDirShadow(DirLight light, vec4 fragPosLightSpace, vec3 normal);
float CalcPointShadow(PointLight light, vec3 fragPos);
float CalcSpotShadow(SpotLight light, vec4 fragPosLightSpace, vec3 normal);

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

void main()
{    
    // properties
    vec3 norm = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    
    // == =====================================================
    // Our lighting is set up in 3 phases: directional, point lights and an optional flashlight
    // For each phase, a calculate function is defined that calculates the corresponding color
    // per lamp. In the main() function we take all the calculated colors and sum them up for
    // this fragment's final color.
    // == =====================================================
    vec3 result = vec3(0.0);
    // phase 1: directional lighting
    for(int i = 0; i < numDirLights; i++)
        result += CalcDirLight(dirLights[i], norm, viewDir);    
    // phase 2: point lights
    for(int i = 0; i < numPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, fs_in.FragPos, viewDir);    
    // phase 3: spot light
    for(int i = 0; i < numSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, fs_in.FragPos, viewDir);    
    
    FragColor = vec4(result, 1.0);
}

// calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }
    if(diff <= 0.0)
        spec = 0.0;
    // combine results
    vec3 ambient = vec3(0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, fs_in.TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, fs_in.TexCoords).x);
    
    // calculate shadow
    float shadow = light.hasShadow ? CalcDirShadow(light, fs_in.FragPosLightSpace[light.shadowIndex], normal) : 0.0;

    vec3 lighting = ambient + (1.0 - shadow) * diffuse + specular * (1.0 - shadow);
    return lighting;
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }
    if(diff <= 0.0)
        spec = 0.0;
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, fs_in.TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, fs_in.TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, fs_in.TexCoords).x);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    // calculate shadow
    float shadow = light.hasShadow ? CalcPointShadow(light, fs_in.FragPos) : 0.0;
    vec3 lighting = ambient + (1.0 - shadow) * diffuse + specular * (1.0 - shadow);
    return lighting;
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }
    if(diff <= 0.0)
        spec = 0.0;
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, fs_in.TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, fs_in.TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, fs_in.TexCoords).x);
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    // calculate shadow
    float shadow = light.hasShadow ? CalcSpotShadow(light, fs_in.FragPosLightSpace[light.shadowIndex], normal) : 0.0;
    vec3 lighting = ambient + (1.0 - shadow) * diffuse + specular * (1.0 - shadow);
    return lighting;
}

float CalcDirShadow(DirLight light, vec4 fragPosLightSpace, vec3 normal)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(light.shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(normal, normalize(light.direction))), 0.005);
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    vec2 texelSize = 1.0 / textureSize(light.shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(light.shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 10.0;

    if (projCoords.z > 1.0) 
        shadow = 0.0;

    return shadow;
}

float CalcPointShadow(PointLight light, vec3 fragPos) 
{
    vec3 fragToLight = fragPos - light.position;
    float closestDepth = texture(light.shadowMap, fragToLight).r;

    closestDepth *= light.farPlane;

    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias = 0.15;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / light.farPlane)) / 25.0;
    for(int i = 0; i < samples; i++) {
        float pcfDepth = texture(light.shadowMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        pcfDepth *= light.farPlane; 
        if(currentDepth - bias > pcfDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);

    return shadow;
}

float CalcSpotShadow(SpotLight light, vec4 fragPosLightSpace, vec3 normal)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(light.shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.00025 * (1.0 - dot(normal, normalize(light.direction))), 0.000005);
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    vec2 texelSize = 1.0 / textureSize(light.shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(light.shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 10.0;

    if (projCoords.z > 1.0) 
        shadow = 0.0;

    return shadow;
}
#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include "hittable.h"
#include "material.h"
#include "ray.h"
#include "color.h"
#include "vec3.h"
#include "threadpool.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include <glm/glm.hpp>
using glm::vec4;

class camera {
    public:
        //ratio of image width over height
        float aspect_ratio = 1.0f;
        //rendered image width in pixel count
        int image_width = 100;
        //count of random samples for each pixel
        int samples_per_pixel = 10;
        //maximum number of ray bounces into scene
        int max_depth = 10;
        // Scene background color
        color  background;

        float  vfov     = 90.0f;
        point3  lookfrom = point3(0,0,0);
        point3  lookat   = point3(0,0,-1);
        vec3    vup      = vec3(0,1,0);

        float defocus_angle = 0;
        float focus_dist = 10;

        /* Public Camera Parameters Here */
        void render(const hittable& world, int num_threads) {
            // Initialize OpenGL context and buffers
            initialize();

            // Collect scene data
            auto primitives = collect_primitives(world);
            auto materials = collect_materials(world);
            auto transforms = collect_transforms(world);
            auto textures = collect_textures(world);
            auto bvh = build_bvh(primitives);

            // Create and bind buffers
            setup_buffers(primitives, materials, transforms, textures, bvh);

            // Setup compute shader
            GLuint computeProgram = setup_compute_shader();

            // Dispatch compute shader
            dispatch_compute_shader(computeProgram);

            // Write output
            write_image();
        }

    private:
        /* Private Camera Variables Here */

        int    image_height;        // Rendered image height
        float pixel_samples_scale; //Color scale factor for a sum of pixel samples
        point3 center;              // Camera center
        point3 pixel00_loc;         // Location of pixel 0, 0
        vec3   pixel_delta_u;       // Offset to pixel to the right
        vec3   pixel_delta_v;       // Offset to pixel below
        vec3   u, v, w;
        vec3   defocus_disk_u;
        vec3   defocus_disk_v;

        void initialize() {
            image_height = int(image_width / aspect_ratio);
            image_height = (image_height < 1) ? 1 : image_height;

            pixel_samples_scale = 1.0 / samples_per_pixel;

            center = lookfrom;

            // Determine viewport dimensions.
            auto theta = degrees_to_radians(vfov);
            auto h = std::tan(theta/2);
            auto viewport_height = 2 * h * focus_dist;
            auto viewport_width = viewport_height * (double(image_width)/image_height);

            //calculate the u,v,w unit basis vectors for the camera coordinate frame
            w = unit_vector(lookfrom - lookat);
            u = unit_vector(cross(vup, w));
            v = cross(w,u);

            //calculate the vectors across the horizontal and down the vertical viewport edges
            vec3 viewport_u = viewport_width * u;
            vec3 viewport_v = viewport_height * -v;

            // Calculate the horizontal and vertical delta vectors from pixel to pixel.
            pixel_delta_u = viewport_u / image_width;
            pixel_delta_v = viewport_v / image_height;

            // Calculate the location of the upper left pixel.
            auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
            pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

            // Calculate the camera defocus disk basis vectors.
            auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
            defocus_disk_u = u * defocus_radius;
            defocus_disk_v = v * defocus_radius;
        }

        GLuint setupComputeShader() {
            const char* computeShaderSource = R"(
                #version 430
                layout(local_size_x = 16, local_size_y = 16) in;

                // Output buffer
                layout(std430, binding = 0) buffer OutputBuffer {
                    vec4 pixels[];
                };

                // Scene data buffers
                layout(std430, binding = 1) buffer SphereBuffer {
                    struct Sphere {
                        vec3 center;
                        float radius;
                        int material_index;
                    } spheres[];
                };

                layout(std430, binding = 2) buffer MaterialBuffer {
                    struct Material {
                        int type;
                        vec3 albedo;
                        float fuzz;
                        float ir;
                    } materials[];
                };

                // Structures needed for ray tracing
                struct Ray {
                    vec3 origin;
                    vec3 direction;
                };

                struct HitRecord {
                    vec3 p;
                    vec3 normal;
                    float t;
                    bool front_face;
                    int material_index;
                };

                // Camera uniforms
                uniform vec3 camera_center;
                uniform vec3 pixel00_loc;
                uniform vec3 pixel_delta_u;
                uniform vec3 pixel_delta_v;
                uniform int image_width;
                uniform int image_height;
                uniform int samples_per_pixel;
                uniform int max_depth;
                uniform vec3 background;

                // Random number generation
                uint seed = uint(
                    uint(gl_GlobalInvocationID.x) * 1973u + 
                    uint(gl_GlobalInvocationID.y) * 9277u + 
                    uint(gl_GlobalInvocationID.z) * 26699u
                );

                float random() {
                    seed = seed * 747796405u + 2891336453u;
                    uint result = ((seed >> ((seed >> 28) + 4u)) ^ seed) * 277803737u;
                    result = (result >> 22) ^ result;
                    return float(result) / 4294967295.0;
                }

                vec3 random_in_unit_sphere() {
                    while (true) {
                        vec3 p = vec3(random() * 2.0 - 1.0, 
                                     random() * 2.0 - 1.0, 
                                     random() * 2.0 - 1.0);
                        if (dot(p, p) < 1.0) return p;
                    }
                }

                vec3 random_unit_vector() {
                    return normalize(random_in_unit_sphere());
                }

                void set_face_normal(Ray r, vec3 outward_normal, inout HitRecord rec) {
                    rec.front_face = dot(r.direction, outward_normal) < 0;
                    rec.normal = rec.front_face ? outward_normal : -outward_normal;
                }

                bool hit_sphere(Sphere sphere, Ray r, float t_min, float t_max, inout HitRecord rec) {
                    vec3 oc = r.origin - sphere.center;
                    float a = dot(r.direction, r.direction);
                    float half_b = dot(oc, r.direction);
                    float c = dot(oc, oc) - sphere.radius * sphere.radius;
                    float discriminant = half_b * half_b - a * c;

                    if (discriminant < 0) return false;
                    float sqrtd = sqrt(discriminant);

                    float root = (-half_b - sqrtd) / a;
                    if (root < t_min || t_max < root) {
                        root = (-half_b + sqrtd) / a;
                        if (root < t_min || t_max < root)
                            return false;
                    }

                    rec.t = root;
                    rec.p = r.origin + rec.t * r.direction;
                    vec3 outward_normal = (rec.p - sphere.center) / sphere.radius;
                    set_face_normal(r, outward_normal, rec);
                    rec.material_index = sphere.material_index;

                    return true;
                }

                vec3 ray_color(Ray r, int depth) {
                    if (depth <= 0) return vec3(0.0);

                    HitRecord closest_hit;
                    closest_hit.t = 1e30; // Initialize to a large value
                    bool hit_anything = false;

                    // Check all spheres
                    for (int i = 0; i < spheres.length(); i++) {
                        HitRecord temp_rec;
                        if (hit_sphere(spheres[i], r, 0.001, closest_hit.t, temp_rec)) {
                            hit_anything = true;
                            closest_hit = temp_rec;
                        }
                    }

                    if (hit_anything) {
                        Ray scattered;
                        vec3 attenuation;
                        
                        Material mat = materials[closest_hit.material_index];
                        
                        switch (mat.type) {
                            case 0: // Lambertian
                                {
                                    vec3 scatter_direction = closest_hit.normal + random_unit_vector();
                                    scattered = Ray(closest_hit.p, scatter_direction);
                                    attenuation = mat.albedo;
                                    return attenuation * ray_color(scattered, depth - 1);
                                }
                            case 1: // Metal
                                {
                                    vec3 reflected = reflect(normalize(r.direction), closest_hit.normal);
                                    scattered = Ray(closest_hit.p, reflected + mat.fuzz * random_in_unit_sphere());
                                    attenuation = mat.albedo;
                                    if (dot(scattered.direction, closest_hit.normal) > 0)
                                        return attenuation * ray_color(scattered, depth - 1);
                                    return vec3(0.0);
                                }
                            case 2: // Dielectric
                                {
                                    float refraction_ratio = closest_hit.front_face ? (1.0/mat.ir) : mat.ir;
                                    vec3 unit_direction = normalize(r.direction);
                                    float cos_theta = min(dot(-unit_direction, closest_hit.normal), 1.0);
                                    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
                                    
                                    bool cannot_refract = refraction_ratio * sin_theta > 1.0;
                                    vec3 direction;
                                    
                                    if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random())
                                        direction = reflect(unit_direction, closest_hit.normal);
                                    else
                                        direction = refract(unit_direction, closest_hit.normal, refraction_ratio);
                                    
                                    scattered = Ray(closest_hit.p, direction);
                                    return ray_color(scattered, depth - 1);
                                }
                        }
                    }

                    // Background
                    vec3 unit_direction = normalize(r.direction);
                    float t = 0.5 * (unit_direction.y + 1.0);
                    return (1.0 - t) * vec3(1.0) + t * vec3(0.5, 0.7, 1.0);
                }

                // Add helper functions for dielectric material
                float reflectance(float cosine, float ref_idx) {
                    // Use Schlick's approximation for reflectance
                    float r0 = (1 - ref_idx) / (1 + ref_idx);
                    r0 = r0 * r0;
                    return r0 + (1 - r0) * pow((1 - cosine), 5);
                }

                void main() {
                    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
                    if (pixel.x >= image_width || pixel.y >= image_height) return;

                    int index = pixel.y * image_width + pixel.x;
                    vec3 pixel_color = vec3(0.0);

                    for (int s = 0; s < samples_per_pixel; s++) {
                        vec2 offset = vec2(random(), random());
                        vec3 pixel_sample = pixel00_loc 
                            + (float(pixel.x) + offset.x) * pixel_delta_u 
                            + (float(pixel.y) + offset.y) * pixel_delta_v;
                        
                        vec3 ray_direction = pixel_sample - camera_center;
                        Ray r = Ray(camera_center, ray_direction);
                        pixel_color += ray_color(r, max_depth);
                    }

                    // Divide the color by the number of samples and gamma-correct
                    pixel_color = pixel_color / float(samples_per_pixel);
                    pixel_color = sqrt(pixel_color); // Simple gamma correction

                    pixels[index] = vec4(pixel_color, 1.0);
                }
            )";

            GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
            glShaderSource(computeShader, 1, &computeShaderSource, nullptr);
            glCompileShader(computeShader);

            // Check compilation
            GLint success;
            glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
            if (!success) {
                char infoLog[512];
                glGetShaderInfoLog(computeShader, 512, nullptr, infoLog);
                std::cerr << "Compute shader compilation failed:\n" << infoLog << std::endl;
                return 0;
            }

            GLuint program = glCreateProgram();
            glAttachShader(program, computeShader);
            glLinkProgram(program);
            glDeleteShader(computeShader);

            return program;
        }

        void setUniforms(GLuint program) {
            glUniform3fv(glGetUniformLocation(program, "camera_center"), 1, &center[0]);
            glUniform3fv(glGetUniformLocation(program, "pixel00_loc"), 1, &pixel00_loc[0]);
            glUniform3fv(glGetUniformLocation(program, "pixel_delta_u"), 1, &pixel_delta_u[0]);
            glUniform3fv(glGetUniformLocation(program, "pixel_delta_v"), 1, &pixel_delta_v[0]);
            glUniform1i(glGetUniformLocation(program, "image_width"), image_width);
            glUniform1i(glGetUniformLocation(program, "image_height"), image_height);
            glUniform1i(glGetUniformLocation(program, "samples_per_pixel"), samples_per_pixel);
            glUniform1i(glGetUniformLocation(program, "max_depth"), max_depth);
            glUniform3fv(glGetUniformLocation(program, "background"), 1, &background[0]);
        }

        // Add these structures to match GPU data layout
        struct sphere_gpu_data {
            vec3 center;
            float radius;
            int material_index;
        };

        struct material_gpu_data {
            int type;          // 0 = lambertian, 1 = metal, 2 = dielectric
            vec3 albedo;
            float fuzz;        // for metal
            float ir;          // for dielectric
        };

        // Helper functions to collect scene data
        std::vector<sphere_gpu_data> collect_spheres(const hittable& world) {
            std::vector<sphere_gpu_data> spheres;
            std::unordered_map<shared_ptr<material>, int> material_indices;
            
            // Traverse the world to collect spheres
            if (const sphere* sphere_ptr = dynamic_cast<const sphere*>(&world)) {
                // Direct sphere
                sphere_gpu_data sphere_data;
                sphere_data.center = sphere_ptr->center;
                sphere_data.radius = sphere_ptr->radius;
                sphere_data.material_index = get_material_index(sphere_ptr->mat, material_indices);
                spheres.push_back(sphere_data);
            } 
            else if (const hittable_list* list_ptr = dynamic_cast<const hittable_list*>(&world)) {
                // List of objects
                for (const auto& object : list_ptr->objects) {
                    auto sub_spheres = collect_spheres(*object);
                    spheres.insert(spheres.end(), sub_spheres.begin(), sub_spheres.end());
                }
            }
            // Add support for BVH_node if needed
            
            return spheres;
        }

        std::vector<material_gpu_data> collect_materials(const hittable& world) {
            std::vector<material_gpu_data> materials;
            std::unordered_set<shared_ptr<material>> unique_materials;
            
            // First pass: collect unique materials
            collect_unique_materials(world, unique_materials);
            
            // Second pass: convert to GPU format
            for (const auto& mat : unique_materials) {
                material_gpu_data mat_data;
                
                if (auto lambertian = dynamic_pointer_cast<lambertian_material>(mat)) {
                    mat_data.type = 0;
                    mat_data.albedo = lambertian->albedo;
                    mat_data.fuzz = 0.0f;
                    mat_data.ir = 0.0f;
                }
                else if (auto metal = dynamic_pointer_cast<metal_material>(mat)) {
                    mat_data.type = 1;
                    mat_data.albedo = metal->albedo;
                    mat_data.fuzz = metal->fuzz;
                    mat_data.ir = 0.0f;
                }
                else if (auto dielectric = dynamic_pointer_cast<dielectric_material>(mat)) {
                    mat_data.type = 2;
                    mat_data.albedo = color(1.0f);
                    mat_data.fuzz = 0.0f;
                    mat_data.ir = dielectric->ir;
                }
                
                materials.push_back(mat_data);
            }
            
            return materials;
        }

        // Helper function to collect unique materials
        void collect_unique_materials(const hittable& obj, std::unordered_set<shared_ptr<material>>& materials) {
            if (const sphere* sphere_ptr = dynamic_cast<const sphere*>(&obj)) {
                materials.insert(sphere_ptr->mat);
            }
            else if (const hittable_list* list_ptr = dynamic_cast<const hittable_list*>(&obj)) {
                for (const auto& object : list_ptr->objects) {
                    collect_unique_materials(*object, materials);
                }
            }
            // Add support for BVH_node if needed
        }

        // Helper function to get or assign material index
        int get_material_index(const shared_ptr<material>& mat, std::unordered_map<shared_ptr<material>, int>& material_indices) {
            auto it = material_indices.find(mat);
            if (it != material_indices.end()) {
                return it->second;
            }
            int new_index = material_indices.size();
            material_indices[mat] = new_index;
            return new_index;
        }

        void checkGLError(const char* message) {
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                throw std::runtime_error(std::string(message) + 
                                       ": GL error " + std::to_string(error));
            }
        }

        void writeImageToFile(const std::vector<vec4>& pixels) {
            std::ofstream file("image.ppm");
            if (!file) {
                throw std::runtime_error("Failed to open output file");
            }

            file << "P3\n" << image_width << ' ' << image_height << "\n255\n";
            for (int j = 0; j < image_height; ++j) {
                for (int i = 0; i < image_width; ++i) {
                    const vec4& pixel = pixels[j * image_width + i];
                    file << write_color(color(pixel.x(), pixel.y(), pixel.z()));
                }
            }
            file.close();
        }

        // Add performance monitoring
        void logPerformanceMetrics(const std::chrono::steady_clock::time_point& start_time) {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time).count();
            
            std::clog << "\nRendering completed in " << duration << "ms\n";
            std::clog << "Rays per second: " 
                     << (double)(image_width * image_height * samples_per_pixel) 
                        / (duration / 1000.0) << "\n";
        }

        void setup_buffers(const std::vector<sphere_gpu_data>& primitives,
                          const std::vector<material_gpu_data>& materials,
                          const std::vector<transform_data>& transforms,
                          const std::vector<texture_data>& textures,
                          const bvh_node& bvh);

        void dispatch_compute_shader(GLuint program);
        void write_image();
};

#endif

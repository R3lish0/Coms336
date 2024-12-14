#include "../include/rtweekend.h"

#include "../include/hittable_list.h"
#include "../include/quad.h"
#include "../include/sphere.h"
#include "../include/camera.h"
#include "../include/constant_medium.h"
#include "../include/material.h"
#include "../include/bvh.h"
#include "../include/texture.h"
#include "../include/mesh.h"
#include "../include/triangle.h"


#include <iostream>
#include <thread>

void cornell_box(int num_threads) {
    hittable_list world;

    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    // Cornell box sides
    world.add(make_shared<quad>(point3(555,0,0), vec3(0,0,555), vec3(0,555,0), green));
    world.add(make_shared<quad>(point3(0,0,555), vec3(0,0,-555), vec3(0,555,0), red));
    world.add(make_shared<quad>(point3(0,555,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,0,-555), white));
    world.add(make_shared<quad>(point3(555,0,555), vec3(-555,0,0), vec3(0,555,0), white));

    // Light
    world.add(make_shared<quad>(point3(213,554,227), vec3(130,0,0), vec3(0,0,105), light));

    // Box
    shared_ptr<hittable> box1 = box(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));
    world.add(box1);

    // Glass Sphere
    auto glass = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(190,90,190), 90, glass));

     // Light Sources
    auto empty_material = shared_ptr<material>();
    hittable_list lights;
    lights.add(
        make_shared<quad>(point3(343,554,332), vec3(-130,0,0), vec3(0,0,-105), empty_material));
    lights.add(make_shared<sphere>(point3(190, 90, 190), 90, empty_material));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 600;
    cam.samples_per_pixel = 1000;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.render(world, num_threads, lights);
}

void simple_sphere_scene(int num_threads) {
    // Scene setup
    hittable_list world;
    hittable_list lights;

    // Create materials
    auto red_mat = make_shared<lambertian>(color(0.8, 0.2, 0.2));    // Bright red for sphere
    auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5)); // Gray for ground
    auto light_mat = make_shared<diffuse_light>(color(2, 2, 2));  // Bright white light

    // Add sphere
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, red_mat));

    // Add ground plane
    world.add(make_shared<quad>(point3(-50, 0, -50), vec3(100,0,0), vec3(0,0,100), ground_mat));

    // Add light source
    lights.add(make_shared<quad>(point3(-2, 4, -2), vec3(4,0,0), vec3(0,0,4), light_mat));

    // Camera setup
    camera cam;

    // Basic image settings
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 800;  // Reduced for faster testing
    cam.samples_per_pixel = 1000;
    cam.max_depth = 40;

    // Camera position
    cam.vfov = 30;
    cam.lookfrom = point3(6, 4, 6);  // Moved to a closer position
    cam.lookat = point3(0, 0, 0);    // Still looking at the origin
    cam.vup = vec3(0, 1, 0);

    // No depth of field
    cam.defocus_angle = 0;

    // Sky color
    cam.background = color(0.7, 0.8, 1.0);

    // Render
    cam.render(world, num_threads, lights);
}


void quantum_lab_scene(int num_threads) {
    hittable_list world;
    hittable_list lights;

    // Materials
    auto glass = make_shared<dielectric>(1.5);
    auto tinted_glass = make_shared<dielectric>(1.7);
    auto chrome = make_shared<metal>(color(0.9, 0.9, 1.0), 0.1);  // Less fuzzy chrome
    auto glow_blue = make_shared<diffuse_light>(color(0.2, 0.4, 15.0));  // Reduced from (2.0, 4.0, 50.0)
    auto floor_metal = make_shared<metal>(color(0.7, 0.7, 0.8), 0.1);  // Reflective floor
    auto glow_white = make_shared<diffuse_light>(color(10, 10, 10));   // For accent lights

    // Add glass enclosure around central sphere
    double enclosure_size = 7.0;
    double glass_thickness = 0.2;
    
    // Glass panels (same as before)
    // Top glass panel
    world.add(box(
        point3(-enclosure_size, enclosure_size + 5, -enclosure_size),
        point3(enclosure_size, enclosure_size + 5 + glass_thickness, enclosure_size),
        glass
    ));
    
    // Bottom glass panel
    world.add(box(
        point3(-enclosure_size, 5 - enclosure_size, -enclosure_size),
        point3(enclosure_size, 5 - enclosure_size + glass_thickness, enclosure_size),
        glass
    ));
    
    // Front glass panel
    world.add(box(
        point3(-enclosure_size, 5 - enclosure_size, enclosure_size),
        point3(enclosure_size, 5 + enclosure_size, enclosure_size + glass_thickness),
        glass
    ));
    
    // Back glass panel
    world.add(box(
        point3(-enclosure_size, 5 - enclosure_size, -enclosure_size - glass_thickness),
        point3(enclosure_size, 5 + enclosure_size, -enclosure_size),
        glass
    ));
    
    // Left glass panel
    world.add(box(
        point3(-enclosure_size - glass_thickness, 5 - enclosure_size, -enclosure_size),
        point3(-enclosure_size, 5 + enclosure_size, enclosure_size),
        glass
    ));
    
    // Right glass panel
    world.add(box(
        point3(enclosure_size, 5 - enclosure_size, -enclosure_size),
        point3(enclosure_size + glass_thickness, 5 + enclosure_size, enclosure_size),
        glass
    ));

    // Central quantum containment
    world.add(make_shared<sphere>(point3(0, 5, 0), 5.0, glass));
    world.add(make_shared<sphere>(point3(0, 5, 0), 4.5, tinted_glass));
    world.add(make_shared<sphere>(point3(0, 5, 0), 3.5, glass));
    world.add(make_shared<sphere>(point3(0, 5, 0), 2.0, glow_blue));
    lights.add(make_shared<sphere>(point3(0, 5, 0), 2.0, glow_blue));



    // Orbiting larger metal spheres (with enhanced lighting)
    for(int i = 0; i < 8; i++) {  // Changed from 7 to 8
        // Customize position for each orb to distribute them around the scene
        double radius, height, angle;
        switch(i) {
            case 0: radius = 15.0; height = 8.0; angle = pi/6; break;     // Front right
            case 1: radius = 18.0; height = 12.0; angle = 4*pi/3; break;  // Back left
            case 2: radius = 12.0; height = 15.0; angle = 3*pi/4; break;  // Mid left
            case 3: radius = 20.0; height = 6.0; angle = 7*pi/4; break;   // Back right
            case 4: radius = 16.0; height = 10.0; angle = 3*pi/2; break;  // Back center
            case 5: radius = 14.0; height = 5.0; angle = pi/2; break;     // Front center
            case 6: radius = 17.0; height = 7.0; angle = pi; break;       // Left side
            case 7: radius = 19.0; height = 9.0; angle = 5*pi/4; break;   // New: Back left corner
        }
        
        point3 center(radius * cos(angle), height, radius * sin(angle));
        
        // Create chrome-like metal sphere with slightly darker base color and tiny fuzz
        auto sphere_mat = make_shared<metal>(
            color(0.7, 0.7, 0.8),  // Slightly darker base color
            0.1                    // Small amount of fuzz to break up perfect reflections
        );
        world.add(make_shared<sphere>(center, 1.5, sphere_mat));

        // Create glowing ring around each sphere with motion blur
        double ring_radius = 2.4;
        int ring_segments = 20;
        
        for(int j = 0; j < ring_segments; j++) {
            double ring_angle = j * (2 * pi / ring_segments);
            // Calculate a slightly different angle for the end position
            double ring_angle_end = ring_angle + (pi/12);  // Rotate by 15 degrees
            
            // Start position
            double ring_x = cos(ring_angle) * ring_radius;
            double ring_z = sin(ring_angle) * ring_radius;
            point3 start_pos = center + vec3(ring_x, 0, ring_z);
            
            // End position
            double ring_x_end = cos(ring_angle_end) * ring_radius;
            double ring_z_end = sin(ring_angle_end) * ring_radius;
            point3 end_pos = center + vec3(ring_x_end, 0, ring_z_end);
            
            auto ring_light = make_shared<diffuse_light>(color(3.0, 3.0, 3.5));
            // Use moving sphere constructor (start_pos, end_pos, radius, material)
            auto ring_segment = make_shared<sphere>(start_pos, end_pos, 0.2, ring_light);
            world.add(ring_segment);
            lights.add(make_shared<sphere>(start_pos, end_pos, 0.2, nullptr));
        }
    }


    // Add metal pylons at corners with glowing bases
    for(int i = 0; i < 4; i++) {
        double angle = i * (2 * pi / 4);
        double radius = 10.0;
        point3 base(radius * cos(angle), 0, radius * sin(angle));
        
        // Main pylon
        shared_ptr<hittable> pylon = box(point3(-0.5,-0.5,-0.5), point3(0.5,8,0.5), chrome);
        auto moved_pylon = make_shared<translate>(pylon, vec3(base.x(), 0, base.z()));
        world.add(moved_pylon);

        // Glowing base for each pylon
        shared_ptr<hittable> base_light = box(point3(-1,-0.1,-1), point3(1,0,1), glow_white);
        auto moved_base = make_shared<translate>(base_light, vec3(base.x(), 0, base.z()));
        world.add(moved_base);
        lights.add(make_shared<quad>(point3(base.x()-1, 0, base.z()-1), 
                                   vec3(2,0,0), vec3(0,0,2), nullptr));
    }

    // Add reflective platform with geometric patterns
    // Main platform - increased size significantly
    world.add(make_shared<quad>(point3(-100, -0.1, -100), 
                               vec3(200,0,0), 
                               vec3(0,0,200), 
                               floor_metal));

    // Add concentric rings in the floor
    for(int ring = 1; ring <= 3; ring++) {
        double ring_radius = ring * 4.0;
        int segments = 16 * ring;  // More segments for outer rings
        
        for(int i = 0; i < segments; i++) {
            double angle1 = i * (2 * pi / segments);
            double angle2 = (i + 1) * (2 * pi / segments);
            
            point3 p1(ring_radius * cos(angle1), 0.01, ring_radius * sin(angle1));
            point3 p2(ring_radius * cos(angle2), 0.01, ring_radius * sin(angle2));
            
            // Create thin metal strip
            vec3 direction = p2 - p1;
            vec3 width(0, 0, 0.2);
            
            world.add(make_shared<quad>(p1, direction, width, chrome));
        }
    }

    // Add distant stars (add this in quantum_lab_scene before the camera setup)
    for(int i = 0; i < 200; i++) {  // Increased count since we're targeting a smaller area
        // Randomize star brightness
        double brightness = random_double(0.5, 2.0);
        auto star_mat = make_shared<diffuse_light>(color(brightness, brightness, brightness));
        
        // Generate positions in a cone-like shape behind the scene
        // Center around the negative of the camera's direction
        double x = random_double(-75, 75);  // Spread horizontally
        double y = random_double(-10, 100);   // Mostly above the scene
        double z = random_double(-50, 50);  // Spread in depth
        
        // Shift the distribution to be behind the quantum containment
        point3 star_pos(
            x - 30,     // Shift left of camera
            y + 10,     // Raise up
            z - 60      // Shift behind scene
        );
        
        // Randomize star size
        double star_size = random_double(0.3, 0.7);
        
        world.add(make_shared<sphere>(star_pos, star_size, star_mat));
        lights.add(make_shared<sphere>(star_pos, star_size, nullptr));
    } 

    // Camera setup
    camera cam;

    cam.aspect_ratio = 1;
    cam.image_width = 200;
    cam.samples_per_pixel = 100;
    cam.max_depth = 40;

    cam.vfov = 45;
    cam.lookfrom = point3(30, 25, 30);
    cam.lookat = point3(0, 10, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.4;
    cam.focus_dist = 30.0;
    
    cam.background = color(0.0, 0.0, 0.0);


    cam.render(world, num_threads, lights);
}

void instancing_demo_scene(int num_threads) {
    hittable_list world;
    hittable_list lights;

    // Create materials
    auto red_mat = make_shared<lambertian>(color(0.8, 0.2, 0.2));    // Red
    auto blue_mat = make_shared<lambertian>(color(0.2, 0.2, 0.8));   // Blue
    auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5)); // Gray ground
    auto light_mat = make_shared<diffuse_light>(color(4, 4, 4));     // Bright white light

    // Create a single box that we'll instance
    shared_ptr<hittable> template_box = box(point3(-0.5, -0.5, -0.5), point3(0.5, 0.5, 0.5), red_mat);

    // Create multiple instances with different transforms
    for(int i = 0; i < 5; i++) {
        // Calculate position in a circle
        double angle = i * (2 * pi / 5);
        double radius = 3.0;
        vec3 position(radius * cos(angle), 0, radius * sin(angle));
        
        // Rotate each box differently
        auto rotated_box = make_shared<rotate_y>(template_box, angle * (180/pi));
        auto moved_box = make_shared<translate>(rotated_box, position);
        
        world.add(moved_box);
    }

    // Add center blue box for contrast
    auto center_box = make_shared<translate>(
        box(point3(-0.5, -0.5, -0.5), point3(0.5, 0.5, 0.5), blue_mat),
        vec3(0, 0, 0)
    );
    world.add(center_box);

    // Add ground plane
    world.add(make_shared<quad>(point3(-10, -0.5, -10), 
                               vec3(20,0,0), 
                               vec3(0,0,20), 
                               ground_mat));

    // Add overhead light
    auto light = make_shared<quad>(point3(-2, 5, -2), 
                                  vec3(4,0,0), 
                                  vec3(0,0,4), 
                                  light_mat);
    world.add(light);
    lights.add(light);

    // Camera setup
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 45;
    cam.lookfrom = point3(0, 6, 12);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.background = color(0.1, 0.1, 0.1);

    cam.render(world, num_threads, lights);
}

void brdf_demo_scene(int num_threads) {
    hittable_list world;
    hittable_list lights;

    // Create materials showing different BRDFs
    auto diffuse_red = make_shared<lambertian>(color(0.8, 0.2, 0.2));     // Diffuse BRDF
    auto metal_smooth = make_shared<metal>(color(0.8, 0.8, 0.8), 0.0);    // Perfect specular BRDF
    auto metal_rough = make_shared<metal>(color(0.8, 0.8, 0.8), 0.4);     // Rough specular BRDF
    auto glass = make_shared<dielectric>(1.5);                            // Fresnel BRDF
    auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5));     // Ground
    auto light_mat = make_shared<diffuse_light>(color(4, 4, 4));         // Light source

    // Add spheres with different materials
    world.add(make_shared<sphere>(point3(-3, 1, 0), 1.0, diffuse_red));     // Diffuse sphere
    world.add(make_shared<sphere>(point3(-1, 1, 0), 1.0, metal_smooth));    // Perfect mirror
    world.add(make_shared<sphere>(point3(1, 1, 0), 1.0, metal_rough));      // Rough metal
    world.add(make_shared<sphere>(point3(3, 1, 0), 1.0, glass));            // Glass sphere

    // Add ground plane
    world.add(make_shared<quad>(point3(-10, 0, -10), 
                               vec3(20,0,0), 
                               vec3(0,0,20), 
                               ground_mat));

    // Add two lights to better show the BRDFs
    auto light1 = make_shared<quad>(point3(-2, 5, -2), 
                                   vec3(4,0,0), 
                                   vec3(0,0,4), 
                                   light_mat);
    world.add(light1);
    lights.add(light1);

    // Add a smaller light to the side
    auto light2 = make_shared<quad>(point3(-4, 3, 2), 
                                   vec3(2,0,0), 
                                   vec3(0,2,0), 
                                   light_mat);
    world.add(light2);
    lights.add(light2);

    // Camera setup
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 500;  // More samples for cleaner BRDF demonstration
    cam.max_depth = 50;

    cam.vfov = 30;
    cam.lookfrom = point3(0, 3, 8);
    cam.lookat = point3(0, 1, 0);
    cam.vup = vec3(0, 1, 0);

    // Small depth of field to enhance visual appeal
    cam.defocus_angle = 0.3;
    cam.focus_dist = 10.0;

    cam.background = color(0.1, 0.1, 0.1);

    cam.render(world, num_threads, lights);
}

void materials_and_textures_demo(int num_threads) {
    hittable_list world;
    hittable_list lights;

    // Create textures
    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));
    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto marble_texture = make_shared<noise_texture>(4);
    auto wood_texture = make_shared<image_texture>("wood-texture.jpg");
    
    // Basic materials
    auto diffuse_red = make_shared<lambertian>(color(0.8, 0.2, 0.2));     // Simple diffuse
    auto diffuse_checker = make_shared<lambertian>(checker);               // Textured diffuse
    auto diffuse_earth = make_shared<lambertian>(earth_texture);          // Image textured
    auto diffuse_marble = make_shared<lambertian>(marble_texture);        // Noise textured
    auto diffuse_wood = make_shared<lambertian>(wood_texture);           // Wood texture
    
    // Metals with different roughness
    auto metal_smooth = make_shared<metal>(color(0.8, 0.8, 0.8), 0.0);    // Perfect mirror
    auto metal_rough = make_shared<metal>(color(0.8, 0.8, 0.8), 0.4);     // Rough metal
    auto gold = make_shared<metal>(color(0.8, 0.6, 0.2), 0.1);           // Gold-like
    auto copper = make_shared<metal>(color(0.8, 0.5, 0.3), 0.2);         // Copper-like
    
    // Dielectrics (glass)
    auto glass = make_shared<dielectric>(1.5);                           // Standard glass
    auto diamond = make_shared<dielectric>(2.4);                         // Diamond-like
    
    // Emissive materials
    auto light_white = make_shared<diffuse_light>(color(4, 4, 4));      // White light
    auto light_warm = make_shared<diffuse_light>(color(4, 3, 2));       // Warm light
    auto light_blue = make_shared<diffuse_light>(color(2, 2, 4));       // Blue light

    // Ground plane with checker pattern
    world.add(make_shared<quad>(point3(-15, -1, -15), 
                               vec3(30,0,0), 
                               vec3(0,0,30), 
                               diffuse_checker));

    // Row 1: Basic materials
    world.add(make_shared<sphere>(point3(-8, 1, 0), 1.0, diffuse_red));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, diffuse_earth));
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, diffuse_marble));
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, diffuse_wood));

    // Row 2: Metals
    world.add(make_shared<sphere>(point3(-8, 1, 4), 1.0, metal_smooth));
    world.add(make_shared<sphere>(point3(-4, 1, 4), 1.0, metal_rough));
    world.add(make_shared<sphere>(point3(0, 1, 4), 1.0, gold));
    world.add(make_shared<sphere>(point3(4, 1, 4), 1.0, copper));

    // Row 3: Glass and emissive
    world.add(make_shared<sphere>(point3(-8, 1, 8), 1.0, glass));
    world.add(make_shared<sphere>(point3(-4, 1, 8), 1.0, diamond));
    
    // Small emissive spheres
    world.add(make_shared<sphere>(point3(0, 1, 8), 1.0, light_warm));
    world.add(make_shared<sphere>(point3(4, 1, 8), 1.0, light_blue));

    // Add overhead area light
    auto overhead_light = make_shared<quad>(point3(-5, 8, -5), 
                                          vec3(10,0,0), 
                                          vec3(0,0,10), 
                                          light_white);
    world.add(overhead_light);
    lights.add(overhead_light);

    // Add small accent lights
    lights.add(make_shared<sphere>(point3(0, 1, 8), 1.0, nullptr));  // Warm light
    lights.add(make_shared<sphere>(point3(4, 1, 8), 1.0, nullptr));  // Blue light

    // Camera setup
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 500;  // High sample count for clean material showcase
    cam.max_depth = 50;

    cam.vfov = 30;
    cam.lookfrom = point3(0, 12, 20);
    cam.lookat = point3(0, 0, 4);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.3;
    cam.focus_dist = 20.0;

    cam.background = color(0.1, 0.1, 0.1);

    cam.render(world, num_threads, lights);
}

void quad_demo_scene(int num_threads) {
    hittable_list world;
    hittable_list lights;

    // Create materials
    auto red_mat = make_shared<lambertian>(color(0.8, 0.2, 0.2));     // Red
    auto blue_mat = make_shared<lambertian>(color(0.2, 0.2, 0.8));    // Blue
    auto green_mat = make_shared<lambertian>(color(0.2, 0.8, 0.2));   // Green
    auto white_mat = make_shared<lambertian>(color(0.8, 0.8, 0.8));   // White
    auto metal_mat = make_shared<metal>(color(0.8, 0.8, 0.8), 0.1);   // Metallic
    auto light_mat = make_shared<diffuse_light>(color(4, 4, 4));      // Light

    // Create a "room" with quads
    // Floor
    world.add(make_shared<quad>(point3(-5, -2, -5), 
                               vec3(10, 0, 0), 
                               vec3(0, 0, 10), 
                               white_mat));
    
    // Back wall
    world.add(make_shared<quad>(point3(-5, -2, 5), 
                               vec3(10, 0, 0), 
                               vec3(0, 10, 0), 
                               blue_mat));
    
    // Right wall
    world.add(make_shared<quad>(point3(5, -2, -5), 
                               vec3(0, 0, 10), 
                               vec3(0, 10, 0), 
                               red_mat));

    // Left wall
    world.add(make_shared<quad>(point3(-5, -2, -5), 
                               vec3(0, 0, 10), 
                               vec3(0, 10, 0), 
                               green_mat));

    // Ceiling light
    auto ceiling_light = make_shared<quad>(point3(-2, 8, -2), 
                                         vec3(4, 0, 0), 
                                         vec3(0, 0, 4), 
                                         light_mat);
    world.add(ceiling_light);
    lights.add(ceiling_light);

    // Add some floating quads to show orientation
    // Metallic quad at an angle
    world.add(make_shared<quad>(point3(-2, 0, 0), 
                               vec3(2, 1, 1), 
                               vec3(-1, 2, 1), 
                               metal_mat));

    // Vertical quad
    world.add(make_shared<quad>(point3(2, 0, -2), 
                               vec3(0, 3, 0), 
                               vec3(-2, 0, 2), 
                               blue_mat));

    // Camera setup
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 200;
    cam.max_depth = 50;

    cam.vfov = 80;
    cam.lookfrom = point3(0, 4, -8);
    cam.lookat = point3(0, 2, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.background = color(0.1, 0.1, 0.1);

    cam.render(world, num_threads, lights);
}

void low_camera_scene(int num_threads) {
    hittable_list world;
    hittable_list lights;

    // Create materials
    auto red_mat = make_shared<lambertian>(color(0.8, 0.2, 0.2));    // Red
    auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5)); // Gray ground
    auto light_mat = make_shared<diffuse_light>(color(4, 4, 4));     // Light

    // Add sphere
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, red_mat));

    // Add ground plane
    world.add(make_shared<quad>(point3(-50, 0, -50), 
                               vec3(100,0,0), 
                               vec3(0,0,100), 
                               ground_mat));

    // Add light source
    auto light = make_shared<quad>(point3(-2, 4, -2), 
                                  vec3(4,0,0), 
                                  vec3(0,0,4), 
                                  light_mat);
    world.add(light);
    lights.add(light);

    // Camera setup - LOW perspective
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    // Low camera position, looking slightly up
    cam.vfov = 30;
    cam.lookfrom = point3(3, 0.5, 3);  // Camera close to ground
    cam.lookat = point3(0, 1, 0);      // Looking at sphere center
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.4;
    cam.focus_dist = 4.0;

    cam.background = color(0.7, 0.8, 1.0);

    cam.render(world, num_threads, lights);
}

void high_camera_scene(int num_threads) {
    hittable_list world;
    hittable_list lights;

    // Create materials
    auto red_mat = make_shared<lambertian>(color(0.8, 0.2, 0.2));    // Red
    auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5)); // Gray ground
    auto light_mat = make_shared<diffuse_light>(color(4, 4, 4));     // Light

    // Add sphere
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, red_mat));

    // Add ground plane
    world.add(make_shared<quad>(point3(-50, 0, -50), 
                               vec3(100,0,0), 
                               vec3(0,0,100), 
                               ground_mat));

    // Add light source
    auto light = make_shared<quad>(point3(-2, 4, -2), 
                                  vec3(4,0,0), 
                                  vec3(0,0,4), 
                                  light_mat);
    world.add(light);
    lights.add(light);

    // Camera setup - HIGH perspective
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    // High camera position, looking down
    cam.vfov = 45;
    cam.lookfrom = point3(4, 6, 4);    // Camera high up
    cam.lookat = point3(0, 0, 0);      // Looking at ground center
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.4;
    cam.focus_dist = 10.0;

    cam.background = color(0.7, 0.8, 1.0);

    cam.render(world, num_threads, lights);
}

void motion_blur_demo_scene(int num_threads) {
    hittable_list world;
    hittable_list lights;

    // Create materials
    auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5));     // Gray ground
    auto metal_mat = make_shared<metal>(color(0.8, 0.8, 0.8), 0.1);      // Metallic spheres
    auto light_mat = make_shared<diffuse_light>(color(4, 4, 4));         // Light
    auto center_mat = make_shared<lambertian>(color(0.7, 0.3, 0.3));     // Center sphere

    // Add ground
    world.add(make_shared<quad>(point3(-50, 0, -50), 
                               vec3(100,0,0), 
                               vec3(0,0,100), 
                               ground_mat));

    // Add stationary center sphere
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, center_mat));

    // Add orbiting spheres with motion blur
    int num_orbiting = 8;
    for(int i = 0; i < num_orbiting; i++) {
        double angle = i * (2 * pi / num_orbiting);
        double next_angle = angle + (pi/6);  // Move 30 degrees
        double radius = 4.0;
        double height = 1.0;

        // Calculate start and end positions
        point3 start_pos(
            radius * cos(angle),
            height,
            radius * sin(angle)
        );
        
        point3 end_pos(
            radius * cos(next_angle),
            height,
            radius * sin(next_angle)
        );

        // Create moving sphere
        world.add(make_shared<sphere>(start_pos, end_pos, 0.3, metal_mat));
    }

    // Add overhead light
    auto light = make_shared<quad>(point3(-2, 8, -2), 
                                  vec3(4,0,0), 
                                  vec3(0,0,4), 
                                  light_mat);
    world.add(light);
    lights.add(light);

    // Camera setup
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 500;  // More samples for cleaner motion blur
    cam.max_depth = 50;

    cam.vfov = 30;
    cam.lookfrom = point3(8, 4, 8);
    cam.lookat = point3(0, 1, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.2;
    cam.focus_dist = 10.0;

    cam.background = color(0.1, 0.1, 0.1);

    cam.render(world, num_threads, lights);
}

void volume_demo_scene(int num_threads) {
    hittable_list world;
    hittable_list lights;

    // Create materials
    auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5));     // Gray ground
    auto light_mat = make_shared<diffuse_light>(color(4, 4, 4));         // Bright light
    auto red_light = make_shared<diffuse_light>(color(4, 0.5, 0.5));     // Red light
    auto blue_light = make_shared<diffuse_light>(color(0.5, 0.5, 4));    // Blue light
    auto glass = make_shared<dielectric>(1.5);                           // Glass material

    // Add ground
    world.add(make_shared<quad>(point3(-50, 0, -50), 
                               vec3(100,0,0), 
                               vec3(0,0,100), 
                               ground_mat));

    // Create a dense fog volume (white)
    auto fog_boundary = make_shared<sphere>(point3(-2, 1, 0), 1.0, glass);
    world.add(make_shared<constant_medium>(fog_boundary, 3.0, color(0.8, 0.8, 0.8)));

    // Create a less dense smoke volume (gray)
    auto smoke_boundary = make_shared<sphere>(point3(2, 1, 0), 1.0, glass);
    world.add(make_shared<constant_medium>(smoke_boundary, 1.5, color(0.5, 0.5, 0.5)));

    // Create a large colored volume (subtle blue)
    auto box1 = box(point3(-3, 0, -3), point3(3, 4, 3), glass);
    world.add(make_shared<constant_medium>(box1, 0.05, color(0.2, 0.4, 0.9)));

    // Add lights for illumination
    // Overhead light
    auto overhead_light = make_shared<quad>(point3(-2, 8, -2), 
                                          vec3(4,0,0), 
                                          vec3(0,0,4), 
                                          light_mat);
    world.add(overhead_light);
    lights.add(overhead_light);

    // Add colored lights on sides for dramatic effect
    auto red_light_source = make_shared<sphere>(point3(-4, 2, -1), 0.5, red_light);
    world.add(red_light_source);
    lights.add(red_light_source);

    auto blue_light_source = make_shared<sphere>(point3(4, 2, -1), 0.5, blue_light);
    world.add(blue_light_source);
    lights.add(blue_light_source);

    // Camera setup
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 200;  // More samples for cleaner volumes
    cam.max_depth = 50;          // Higher depth for multiple scattering

    cam.vfov = 30;
    cam.lookfrom = point3(0, 3, 12);
    cam.lookat = point3(0, 1, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.3;
    cam.focus_dist = 10.0;

    cam.background = color(0.1, 0.1, 0.1);

    cam.render(world, num_threads, lights);
}

void cup_scene(int num_threads) {
    hittable_list world;
    hittable_list lights;

    // Materials
    auto ceramic = make_shared<metal>(color(0.9, 0.9, 0.95), 0.2);    // Ceramic-like material
    auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5));  // Ground
    auto light_mat = make_shared<diffuse_light>(color(4, 4, 4));      // Light

    // Load the cup mesh
    auto cup = make_shared<mesh>("meshes/Nefertiti.obj", ceramic);
    world.add(cup);

    // Ground plane
    world.add(make_shared<quad>(point3(-50, 0, -50), 
                               vec3(100,0,0), 
                               vec3(0,0,100), 
                               ground_mat));

    // Add three-point lighting
    // Main light
    auto main_light = make_shared<quad>(point3(-2, 10, -2), 
                                      vec3(4,0,0), 
                                      vec3(0,0,4), 
                                      light_mat);
    world.add(main_light);
    lights.add(main_light);

    // Fill light
    auto fill_light = make_shared<quad>(point3(-8, 5, 0), 
                                      vec3(0,4,0), 
                                      vec3(0,0,4), 
                                      make_shared<diffuse_light>(color(2, 2, 2)));
    world.add(fill_light);
    lights.add(fill_light);

    // Camera setup
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 30;
    cam.lookfrom = point3(8, 6, 12);
    cam.lookat = point3(0, 2, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.3;
    cam.focus_dist = 10.0;

    cam.background = color(0.2, 0.2, 0.2);

    cam.render(world, num_threads, lights);
}

int main() {

    // Get starting timepoint
    auto start = std::chrono::high_resolution_clock::now();

    unsigned int num_threads = std::thread::hardware_concurrency();

    if (num_threads == 0) {
        std::cout << "Unable to determine number of threads\n";
    }
    else
    {
        std::cout << "We able to rip:" << num_threads << " threads!!\n";
    }




    switch(10) {  // Add new case
        case 1: quantum_lab_scene(num_threads); break;
        case 2: instancing_demo_scene(num_threads); break;
        case 3: brdf_demo_scene(num_threads); break;
        case 4: materials_and_textures_demo(num_threads); break;
        case 5: quad_demo_scene(num_threads); break;
        case 6: low_camera_scene(num_threads); break;
        case 7: high_camera_scene(num_threads); break;
        case 8: motion_blur_demo_scene(num_threads); break;
        case 9: volume_demo_scene(num_threads); break;
        case 10: cup_scene(num_threads); break;
    }





    // Get ending timepoint
    auto stop = std::chrono::high_resolution_clock::now();

    // Get duration. Substart timepoints to
    // get duration. To cast it to proper unit
    // use duration cast method
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(stop - start);

    std::cout << "Time taken by function: "
         << duration.count() << " minutes" << std::endl;

    return 0;


}

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

void figure_1(int num_threads) {
    // Scene setup
    hittable_list world;
    hittable_list lights;

    auto grass_texture = make_shared<image_texture>("grass-texture.jpg");
    auto grass_mat = make_shared<lambertian>(grass_texture);
    auto trunk_texture = make_shared<image_texture>("wood-texture.jpg");
    auto trunk_mat = make_shared<lambertian>(trunk_texture);
    auto building_texture = make_shared<image_texture>("stone-brick.jpg");
    auto building_mat = make_shared<lambertian>(building_texture);
    auto leaves_texture = make_shared<image_texture>("leaves.jpg");
    auto leaves_mat = make_shared<lambertian>(leaves_texture);
    auto road_texture = make_shared<image_texture>("gravel.jpg");
    auto road_mat = make_shared<lambertian>(road_texture);

    // Create materials
    auto red_mat = make_shared<lambertian>(color(0.8, 0.2, 0.2));    // Bright red for truck
    auto sun_mat = make_shared<diffuse_light>(color(180, 96, 36));    // Orange-yellow sun

    // Add truck mesh
    auto truck = make_shared<mesh>("meshes/Cybertruck.obj", red_mat);
    world.add(truck);

    // Add building (tall box) behind the truck
    shared_ptr<hittable> building = box(point3(0,0,0), point3(8,15,4), building_mat);
    auto moved_building = make_shared<translate>(building, vec3(-15, 0, -4));
    world.add(moved_building);
    auto moved_building2 = make_shared<translate>(building, vec3(-15, 0, -14));
    world.add(moved_building2);
    auto moved_building3 = make_shared<translate>(building, vec3(-15, 0, 6));
    world.add(moved_building3);

    // Add tree (trunk and leaves)
    shared_ptr<hittable> trunk = box(point3(0,0,0), point3(1,4,1), trunk_mat);
    auto moved_trunk = make_shared<translate>(trunk, vec3(-8, 0, 4));
    world.add(moved_trunk);

    // Add tree leaves (sphere on top of trunk)
    auto leaves = make_shared<sphere>(point3(-8, 5, 4), 2.5, leaves_mat);
    world.add(leaves);

    // Add tree (trunk and leaves)
    shared_ptr<hittable> trunk2 = box(point3(0,0,0), point3(.5,2.5,.5), trunk_mat);
    auto moved_trunk2 = make_shared<translate>(trunk2, vec3(-4, 0, 8));
    world.add(moved_trunk2);

    // Add tree leaves (sphere on top of trunk)
    auto leaves2 = make_shared<sphere>(point3(-3.5, 2.5, 8), 1.5, leaves_mat);
    world.add(leaves2);

    // Add tree (trunk and leaves)
    shared_ptr<hittable> trunk3 = box(point3(0,0,0), point3(1,3,1), trunk_mat);
    auto moved_trunk3 = make_shared<translate>(trunk3, vec3(-6, 0, -8));
    world.add(moved_trunk3);

    // Add tree leaves (sphere on top of trunk)
    auto leaves3 = make_shared<sphere>(point3(-6, 4, -8), 2, leaves_mat);
    world.add(leaves3);

    // Add multiple overlapping smoke volumes for puffier effect
    auto smoke_boundary1 = make_shared<sphere>(point3(-1.5, 0.3, -5), 1.0,
                                             make_shared<dielectric>(1.5));
    world.add(make_shared<constant_medium>(smoke_boundary1, 1.5, color(0.5, 0.5, 0.5)));

    auto smoke_boundary2 = make_shared<sphere>(point3(-1.7, 0.4, -4.5), 0.8,
                                             make_shared<dielectric>(1.5));
    world.add(make_shared<constant_medium>(smoke_boundary2, 2.0, color(0.6, 0.6, 0.6)));

    auto smoke_boundary3 = make_shared<sphere>(point3(-1.3, 0.2, -5.5), 0.7,
                                             make_shared<dielectric>(1.5));
    world.add(make_shared<constant_medium>(smoke_boundary3, 1.8, color(0.4, 0.4, 0.4)));

    // Add "fire" spheres behind truck with motion and color variation
    for(int i = 0; i < 12; i++) {
        double x_offset = random_double(-0.3, 0.3);
        double y_offset = random_double(-0.2, 0.2);
        double z_offset = random_double(2.5, 4.5);
        double size = random_double(0.05, 0.15);

        auto fire_color = color(
            random_double(3, 5),
            random_double(0.4, 1.6),
            random_double(0.2, 0.4)
        );

        auto fire_mat = make_shared<diffuse_light>(fire_color);


        point3 center1(-1.5 + x_offset, 0.3 + y_offset, -z_offset);
        point3 center2(-1.5 + x_offset - 0.2,
                      0.3 + y_offset + random_double(-0.1, 0.1),
                      -z_offset + random_double(-0.2, 0.2));

        world.add(make_shared<sphere>(center1, center2, size, fire_mat));
        lights.add(make_shared<sphere>(center1, center2, size, fire_mat));
    }

    // Add large grass ground plane
    world.add(make_shared<quad>(point3(-50, -0.1, -50), vec3(100,0,0), vec3(0,0,100), grass_mat));
    // Add road
    world.add(make_shared<quad>(point3(-3, -0.05, -50), vec3(6,0,0), vec3(0,0,100), road_mat));

    // Add sunset sun - repositioned to be visible in camera view
    world.add(make_shared<sphere>(point3(-20, 4, -8), 2.0, sun_mat));
    lights.add(make_shared<sphere>(point3(-20, 4, -8), 2.0, sun_mat));

    // Camera setup
    camera cam;

    // Basic image settings
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 800;
    cam.samples_per_pixel = 200;
    cam.max_depth = 50;

    // Camera position
    cam.vfov = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat = point3(278, 278, 0);
    cam.vup = vec3(0, 1, 0);

    // No depth of field
    cam.defocus_angle = 0;

    // Brighter blue for sky
    cam.background = color(0.4, 0.6, 0.9);

    // Render
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
    cam.image_width = 200;  // Reduced for faster testing
    cam.samples_per_pixel = 1000;
    cam.max_depth = 50;

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
    auto tinted_glass = make_shared<dielectric>(1.7);  // Different refractive index
    auto chrome = make_shared<metal>(color(0.9, 0.9, 1.0), 1);  // Increased reflectivity
    auto glow_blue = make_shared<diffuse_light>(color(0.4, 0.8, 15.0));
    auto floor_mat = make_shared<metal>(color(0.2, 0.2, 0.2), 0.1);
    
    // Add glass enclosure around central sphere
    double enclosure_size = 7.0;  // Size of the glass cage
    double glass_thickness = 0.2;  // Thickness of glass panels
    
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

    // Central quantum containment (now inside glass enclosure)
    world.add(make_shared<sphere>(point3(0, 5, 0), 5.0, glass));
    world.add(make_shared<sphere>(point3(0, 5, 0), 4.5, tinted_glass));
    world.add(make_shared<sphere>(point3(0, 5, 0), 3.5, glass));
    world.add(make_shared<sphere>(point3(0, 5, 0), 2.0, glow_blue));
    lights.add(make_shared<sphere>(point3(0, 5, 0), 2.0, nullptr));

    // Orbiting larger metal spheres (with enhanced lighting)
    for(int i = 0; i < 8; i++) {
        // Customize position for each orb to distribute them around the scene
        double radius, height, angle;
        switch(i) {
            case 0: radius = 15.0; height = 8.0; angle = pi/6; break;     // Front right
            case 1: radius = 18.0; height = 12.0; angle = 4*pi/3; break;  // Back left
            case 2: radius = 12.0; height = 15.0; angle = 3*pi/4; break;  // Mid left
            case 3: radius = 20.0; height = 6.0; angle = 7*pi/4; break;   // Back right
            case 4: radius = 16.0; height = 10.0; angle = pi/2; break;    // Mid front
            case 5: radius = 14.0; height = 13.0; angle = 5*pi/3; break;  // Back mid
            case 6: radius = 22.0; height = 9.0; angle = pi/3; break;     // Far right
            case 7: radius = 19.0; height = 7.0; angle = 3*pi/2; break;   // Far left
        }
        
        point3 center(radius * cos(angle), height, radius * sin(angle));
        
        // Create chrome-like metal sphere with slightly darker base color and tiny fuzz
        auto sphere_mat = make_shared<metal>(
            color(0.7, 0.7, 0.8),  // Slightly darker base color
            0.1                    // Small amount of fuzz to break up perfect reflections
        );
        world.add(make_shared<sphere>(center, 1.5, sphere_mat));

        // Create glowing ring around each sphere
        double ring_radius = 2.4;
        int ring_segments = 20;
        
        for(int j = 0; j < ring_segments; j++) {
            double ring_angle = j * (2 * pi / ring_segments);
            double ring_x = cos(ring_angle) * ring_radius;
            double ring_z = sin(ring_angle) * ring_radius;
            
            vec3 ring_offset(ring_x, 0, ring_z);
            point3 ring_center = center + ring_offset;
            
            auto ring_light = make_shared<diffuse_light>(color(3.0, 3.0, 3.5));  // Back to original brightness
            auto ring_segment = make_shared<sphere>(ring_center, 0.2, ring_light);
            world.add(ring_segment);
            lights.add(make_shared<sphere>(ring_center, 0.2, nullptr));
        }
    }

    // Add volumetric fog effect around the quantum core
    auto fog_boundary = make_shared<sphere>(point3(0, 5, 0), 6.0, make_shared<dielectric>(1.5));
    world.add(make_shared<constant_medium>(fog_boundary, 0.1, color(0.2, 0.4, 0.9)));

    // Add metal pylons at corners
    for(int i = 0; i < 4; i++) {
        double angle = i * (2 * pi / 4);
        double radius = 10.0;
        point3 base(radius * cos(angle), 0, radius * sin(angle));
        
        shared_ptr<hittable> pylon = box(point3(-0.5,-0.5,-0.5), point3(0.5,8,0.5), chrome);
        auto moved_pylon = make_shared<translate>(pylon, vec3(base.x(), 0, base.z()));
        world.add(moved_pylon);
    }

    // Create continuous left-to-right smoke effect
    for(int x = -25; x <= 25; x += 4) {  // Systematic coverage across x-axis
        for(int z = -25; z <= 25; z += 4) {  // Full z-axis coverage
            // Add variation to avoid perfect grid pattern
            double x_offset = random_double(-2, 2);
            double z_offset = random_double(-2, 2);
            
            // Base height varies with x position to create flow effect
            double base_height = sin(x * 0.2) * 1.5;
            
            // Multiple layers of smoke at each position
            for(int layer = 0; layer < 3; layer++) {
                double height = base_height + layer * 0.8 + random_double(-0.5, 0.5);
                double size = random_double(3.0, 5.0);
                
                auto smoke_boundary = make_shared<sphere>(
                    point3(x + x_offset, height, z + z_offset),
                    size,
                    make_shared<dielectric>(1.5)
                );
                world.add(make_shared<constant_medium>(
                    smoke_boundary,
                    0.04,  // Reduced from 0.15
                    color(0.4, 0.4, 0.4)
                ));
            }
        }
    }

    // Add some higher wispy tendrils for vertical variation
    for(int i = 0; i < 30; i++) {
        double x = random_double(-25, 25);
        double z = random_double(-25, 25);
        double height = random_double(2.0, 5.0);
        
        auto smoke_boundary = make_shared<sphere>(
            point3(x, height, z),
            random_double(2.0, 4.0),
            make_shared<dielectric>(1.5)
        );
        world.add(make_shared<constant_medium>(
            smoke_boundary,
            0.03,  // Reduced from 0.1
            color(0.5, 0.5, 0.5)
        ));
    }

    // Add denser smoke near the quantum core
    for(int i = 0; i < 20; i++) {
        double r = random_double(0, 8);
        double angle = random_double(0, 2*pi);
        double x = r * cos(angle);
        double z = r * sin(angle);
        
        auto smoke_boundary = make_shared<sphere>(
            point3(x, random_double(0, 3), z),
            random_double(2.0, 4.0),
            make_shared<dielectric>(1.5)
        );
        world.add(make_shared<constant_medium>(
            smoke_boundary,
            0.05,  // Reduced from 0.2
            color(0.4, 0.4, 0.4)
        ));
    }


    // Add multiple floating cups mixed with the orbs
    auto cup_material = make_shared<lambertian>(color(0.7, 0.7, 0.7));
    
    // Cup 1 - in gap between orbs 0 and 4
    auto cup1 = make_shared<mesh>("meshes/cup_small.obj", cup_material);
    auto rotated_cup1 = make_shared<rotate_y>(cup1, 45);
    auto moved_cup1 = make_shared<translate>(rotated_cup1, vec3(-13, 9, 8));
    world.add(moved_cup1);
    
    // Cup 2 - in gap between orbs 1 and 5
    auto cup2 = make_shared<mesh>("meshes/cup_small.obj", cup_material);
    auto rotated_cup2 = make_shared<rotate_y>(cup2, 120);
    auto moved_cup2 = make_shared<translate>(rotated_cup2, vec3(8, 11, -15));
    world.add(moved_cup2);
    
    // Cup 3 - in gap between orbs 6 and 7
    auto cup3 = make_shared<mesh>("meshes/cup_small.obj", cup_material);
    auto rotated_cup3 = make_shared<rotate_y>(cup3, -30);
    auto moved_cup3 = make_shared<translate>(rotated_cup3, vec3(15, 8, -5));
    world.add(moved_cup3);

    // Camera setup with closer zoom
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 800;
    cam.samples_per_pixel = 500;
    cam.max_depth = 20;

    cam.vfov = 45;
    cam.lookfrom = point3(25, 20, 25);  // Moved closer (was 35, 30, 35)
    cam.lookat = point3(0, 10, 0);      // Keeping same look target
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.4;
    cam.focus_dist = 30.0;
    
    cam.background = color(0.02, 0.02, 0.04);

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




    switch(2) {  // Change to case 1
        case 1: simple_sphere_scene(num_threads); break;
        case 2: quantum_lab_scene(num_threads); break;
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

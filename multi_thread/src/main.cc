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
    cam.lookfrom = point3(12, 2, 0);
    cam.lookat = point3(0, 2, 0);
    cam.vup = vec3(0, 1, 0);

    // No depth of field
    cam.defocus_angle = 0;

    // Brighter blue for sky
    cam.background = color(0.4, 0.6, 0.9);

    // Render
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




    switch(7) {
        case 7: figure_1(num_threads);       break;
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

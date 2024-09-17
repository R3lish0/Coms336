#ifndef CAMERA_H
#define CAMERA_H


#include "material.h"
#include "hittable.h"
#include "ray.h"
#include "color.h"
#include "vec3.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>


class camera {
    public:
        double aspect_ratio = 1.0;  //ratio of image width over height
        int image_width = 100;      //rendered image width in pixel count
        int samples_per_pixel = 10; //count of random samples for each pixel
        int max_depth = 10;         //maximum number of ray bounces into scene
        

        double  vfov     = 90;
        point3  lookfrom = point3(0,0,0);
        point3  lookat   = point3(0,0,-1);
        vec3    vup      = vec3(0,1,0);

        double defocus_angle = 0;
        double focus_dist = 10;


        /* Public Camera Parameters Here */
        void render(const hittable& world) {
            initialize();

            std::ofstream file("image.ppm");

            std::string** output = new std::string*[image_height];  // Allocate rows
                for (int i = 0; i < image_height; ++i) {
                output[i] = new std::string[image_width];  // Allocate columns for each row
            }


            file << "P3\n" << image_width << ' ' << image_height << "\n255\n";

            for (int j = 0; j < image_height;) {
                {
                    //Progress updater
                    std::clog << "\rScanlines left: " << (image_height-j) << " " << std::flush;

                    int thread_target = j;
                    render_line(world, output, thread_target);
                    j++;
                }


            }



//            for (int j = 0; j < image_height; j++) {
//                int thread_target = j;
//                threads.emplace_back(([this, &world, output, thread_target]() 
//                                { render_line(world, output, thread_target); }));
//                }
//
//
//            int scanlines = image_height;
//            for(std::thread& t : threads) {
//                std::clog << "\rScanlines left: " << (scanlines--) << std::flush;
//                t.join();
//                }







            

            for (int i = 0; i < image_height; i++)
            {
                for(int j = 0; j < image_width; j++)
                {
                    file << output[i][j];
                }
            }


            std::clog << "\nDone!\n";
            file.close();
        }


        void render_line(const hittable& world, std::string **output, int j)
        {

                std::vector<std::thread> sample_threads;
                for (int i = 0; i < image_width; i++) {
                    color pixel_color(0,0,0);
                    color color_arr[samples_per_pixel];
                    for (int sample = 0; sample < samples_per_pixel; sample++) 
                    {
                        sample_color(world, i, j, sample, color_arr); 

                    }

                    for (int sample = 0; sample < samples_per_pixel; sample++) {
                        pixel_color += color_arr[sample];
                    }

                    output[j][i] = write_color(pixel_samples_scale * pixel_color);
                }
        }

        void sample_color(const hittable& world, int i, int j,
                int sample, color color_arr[])
        {

            ray r = get_ray(i,j);
            ray_color(r, max_depth, world);
        }

        


    private:
        /* Private Camera Variables Here */

        int    image_height;        // Rendered image height
        double pixel_samples_scale; //Color scale factor for a sum of pixel samples
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

        ray get_ray(int i, int j) const {
        // Construct a camera ray originating from the defocus disk and directed at a randomly
        // sampled point around the pixel location i, j.
        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
        }

        vec3 sample_square() const {
            // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
            return vec3(random_double() - 0.5, random_double() - 0.5, 0);
        }

        

        
        point3 defocus_disk_sample() const {
            // Returns a random point in the camera defocus disk.
            auto p = random_in_unit_disk();
            return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
        }


        color ray_color(const ray& r, int depth,  const hittable& world) const {

            if(depth <= 0)
                return color(0,0,0);

            hit_record rec;

            if (world.hit(r, interval(0.001, infinity), rec)) {
                ray scattered;
                color attenuation;
                if(rec.mat->scatter(r, rec, attenuation, scattered))
                {
                    return attenuation * ray_color(scattered, depth-1,world);
                }
                return color(0,0,0);
            }

            vec3 unit_direction = unit_vector(r.direction());
            auto a = 0.5*(unit_direction.y() + 1.0);
            return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);

        }
};

#endif

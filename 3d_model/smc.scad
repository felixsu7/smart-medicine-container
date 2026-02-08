use <./stepper.scad>;
use <./smooth_prim.scad>;

$fn = 50;

INCHES = 25.4;
wall_thickness = 3;

// FIXME: diameter is actually diameter

base_width = 6 * INCHES;
base_depth = 6 * INCHES;
base_height = 2.5 * INCHES;

base_top_hole_diameter = 2 * INCHES;

LCD_width = 69;
LCD_depth = 4;
LCD_height = 49;

button_height = wall_thickness;
button_diameter = 12;

base_container_spacing = 0.1 * INCHES;

container_height = 2 * INCHES;
container_diameter = 5.5 * INCHES;

container_outer_slope_slant = 17.5;

container_hole_diameter = 1.3 * INCHES;
container_hole_y_offset = -8; //-0.4 * INCHES;
container_hole_thickness = 1;

antenna_diameter = 8;

lid_thickness = 6;

lid_overhang_thickness = 2;
lid_overhang_height = 4;
lid_opening_ratio = 0.35;
lid_diameter = container_diameter / 2 - lid_overhang_thickness - wall_thickness;
lid_sensor_opening_x_offset = container_diameter / 2 * lid_opening_ratio + container_diameter * 0.15;
lid_overhang_thickness_slop = 0.1;

divider_thickness = 2;
divider_height = container_height - 2.3;
divider_slant = 4;

compartment_slope_start_r = 25;
compartment_slope_end_r = 22;

module button() {
  cylinder(button_height + 0.005, button_diameter / 2, button_diameter / 2);
}

module pcb_mount() {
  difference() {
    cube([120 + wall_thickness, 80 + wall_thickness, 7]);
    translate([wall_thickness / 2, wall_thickness / 2, -0.01])
      cube([120, 80, 7.02]);
  }
}

module breadboard_mount() {
  difference() {
    cube([51 * 2.1, 81, 10]);
  }
}

module base() {
  translate([wall_thickness * 2 + base_width / 16, wall_thickness * 2 + base_depth / 3, wall_thickness + 0.001])
    pcb_mount();

  //  translate([wall_thickness * 2 + base_width / 16, wall_thickness * 5, wall_thickness + 0.001])
  //  breadboard_mount();

  color("red")
    difference() {
      SmoothCube([base_width, base_depth, base_height], 5);

      // hollow 
      translate([wall_thickness, wall_thickness, wall_thickness])
        cube([base_width - wall_thickness * 2, base_depth, base_height - wall_thickness * 2]);

      // hallow front for LCD
      translate([base_width / 2 - LCD_width / 2, -0.001, base_height / 2 - LCD_height / 2])
        cube([LCD_width, LCD_depth + 0.002, LCD_height]);

      // hallow front for top button
      translate([base_width * 0.85, wall_thickness + 0.0025, base_height / 2 + INCHES / 2])
        rotate([90, 0, 0])
          button();

      // hallow front for bottom button
      translate([base_width * 0.85, wall_thickness + 0.0025, base_height / 2 - INCHES / 2])
        rotate([90, 0, 0])
          button();

      // hallow top for connection to container itself 
      translate([base_width / 2, base_depth / 2 + container_hole_y_offset, base_height - wall_thickness - 0.001])
        cylinder(d=28, h=50);

      // hallow top for antenna
      translate([base_width * 0.1, base_depth * 0.9, base_height - wall_thickness - 0.001])
        cylinder(wall_thickness + 0.001, antenna_diameter / 2, antenna_diameter / 2);

      translate([base_width / 2, base_depth / 2, base_height])
        translate([0, 0, -1.5])
          cylinder(container_height, container_diameter / 2 + 0.25, container_diameter / 2 + 0.25);
    }

  //container mounting
  difference() {
    translate([base_width / 2, base_depth / 2, base_height - 1 - 0.1]) {
      cube([container_diameter, 1, 2], center=true);
      cube([1, container_diameter, 2], center=true);
    }

    translate([base_width / 2, base_depth / 2 + container_hole_y_offset, base_height - wall_thickness - 0.005])
      cylinder(d=28, h=50);
  }

  // backcover stopper
  translate([wall_thickness, base_depth - wall_thickness * 2, wall_thickness])
    difference() {
      cube([base_width - wall_thickness * 2, wall_thickness, base_height - wall_thickness * 2]);
      translate([wall_thickness, -1, wall_thickness])
        cube([base_width - wall_thickness * 4, wall_thickness * 2 + 2, base_height - wall_thickness * 4]);
    }

  //footings
  translate([base_width * 0.15, base_depth * 0.15, -2.5])
    cylinder(2.6, 5, 5);

  translate([base_width * 0.85, base_depth * 0.15, -2.5])
    cylinder(2.6, 5, 5);
  translate([base_width * 0.15, base_depth * 0.85, -2.5])
    cylinder(2.6, 5, 5);
  translate([base_width * 0.85, base_depth * 0.85, -2.5])
    cylinder(2.6, 5, 5);
}

module base_backcover() {
  cube([base_width - wall_thickness * 2, wall_thickness, base_height - wall_thickness * 2]);
}

module stepper_profile() {
  cylinder(d=28, h=50, $fn=$fn * 2);

  translate([-14.6 / 2, -17, 0])
    cube([14.6, 5.8, 50]);
}

module container_pillar() {
  difference() {
    scale([1.1, 1.1, 0.99])
      stepper_profile();

    translate([0, 0, -0.01])
      scale([1.025, 1.025, 1])
        stepper_profile();

    translate([0, 0, container_height - 1.299])
      rotate([0, 180, 0])
        stepper_28byj48();

    translate([-14.6 / 2, -18, -17 + container_height])
      cube([14.6, 5.8, 100]);

    translate([0, 0, -0.01])
      cylinder(6.01, 100, 100);

    translate([-1.75, -container_hole_diameter / 2 - 10, container_height - wall_thickness - 1.5])
      cube([1.75 * 2, 10, 3.5]);
  }
  /*
  translate([0, 0, container_height - 1.299])
    rotate([0, 180, 0])
      stepper_28byj48();
  */
}

module container_dividers() {
  difference() {
    for (i = [45 / 2 + 0:45:360])
      translate([0, 0, wall_thickness])
        rotate([90, 0, i])
          linear_extrude(container_diameter / 2 - wall_thickness - 0.01)
            polygon([[-divider_slant, 0], [divider_thickness + divider_slant, 0], [divider_thickness, divider_height - wall_thickness - 0.01], [0, divider_height - wall_thickness - 0.01], [-divider_slant, 0]]);
    //cube([container_diameter - wall_thickness * 2.5 - 0.01, divider_thickness, divider_height], center=true);

    // pillar hollow
    translate([0, container_hole_y_offset, -1])
      scale([1.15, 1.15, 1.1])
        stepper_profile();

    // lid overhang entrance
    translate([0, 0, container_height - lid_overhang_height])
      difference() {
        cylinder(lid_overhang_height, container_diameter / 2 - wall_thickness + 0.01, container_diameter / 2 - wall_thickness + 0.01);

        cylinder(lid_overhang_height, container_diameter / 2 - wall_thickness - lid_overhang_thickness, container_diameter / 2 - wall_thickness - lid_overhang_thickness);
      }
  }
}

module mount_tabs() {
  difference() {
    hull() {
      translate([35 / 2, 0, 0])
        cylinder(d=7, 0.8);

      translate([-35 / 2, 0, 0])
        cylinder(d=7, 0.8);
    }
    translate([35 / 2, 0, -0.1])
      cylinder(d=3.5, 0.8 + 0.2);

    translate([-35 / 2, 0, -0.1])
      cylinder(d=3.5, 0.8 + 0.2);
  }
}

module container_slope() {
  difference() {
    cylinder(container_height - wall_thickness * 1.5, compartment_slope_start_r, compartment_slope_end_r);

    // pillar hollow
    translate([0, 0, -0.01])
      scale([1.1, 1.1, 0.99])
        stepper_profile();

    translate([0, 0, container_height - wall_thickness - 1.495])
      rotate([0, 180, 0])
        mount_tabs();

    translate([-1.75, -container_hole_diameter / 2 - 10, container_height - wall_thickness - 1.495 * 3])
      cube([1.75 * 2, 10, 3]);
  }
}

module container() {

  color("blue")
    difference() {
      cylinder(container_height, container_diameter / 2, container_diameter / 2);

      // hallow
      translate([0, 0, wall_thickness - 0.001])
        cylinder(container_height, container_diameter / 2 - wall_thickness, container_diameter / 2 - wall_thickness);

      //hole
      translate([0, container_hole_y_offset, -wall_thickness - 0.001])
        cylinder(d=28, h=50);

      // tightening
      difference() {
        translate([0, 0, 0.499]) {
          cube([container_diameter, 1, 2], center=true);
          cube([1, container_diameter, 2], center=true);
        }

        translate([base_width / 2, base_depth / 2, base_height - wall_thickness - 0.005])
          cylinder(d=28, h=50);
      }
    }

  color("orange")
    translate([0, container_hole_y_offset, 0.002])
      container_pillar();

  // dividers
  color("pink")
    container_dividers();

  //slope
  color("green")
    translate([0, container_hole_y_offset, wall_thickness]) container_slope();

  //outer slope
  color("green")
    translate([0, 0, wall_thickness])
      difference() {
        cylinder(container_height - lid_overhang_height - wall_thickness - 0.01, container_diameter / 2 - wall_thickness, container_diameter / 2 - wall_thickness);

        translate([0, 0, -lid_overhang_height + 0.01])
          cylinder(container_height, container_diameter / 2 - wall_thickness - container_outer_slope_slant, container_diameter / 2 - wall_thickness);
      }
}

module lid() {
  color("yellow")
    difference() {
      union() {
        intersection() {
          rotate_extrude(angle=(360 / 8) * 7 + 5)
            translate([2, 0, 0])
              polygon(points=[[0, 0], [lid_diameter - lid_overhang_thickness + lid_overhang_thickness_slop, 0], [lid_diameter - lid_overhang_thickness + lid_overhang_thickness_slop, -lid_overhang_height], [lid_diameter - lid_overhang_thickness_slop, -lid_overhang_height], [lid_diameter - lid_overhang_thickness_slop, lid_thickness], [0, lid_thickness], [0, 0]]);

          translate([0, 0, -lid_overhang_height])
            cylinder(lid_thickness + lid_overhang_height, lid_diameter + lid_overhang_thickness, lid_diameter + lid_overhang_thickness);
        }

        // opening
        cylinder(lid_thickness, container_diameter * lid_opening_ratio / 2, container_diameter * lid_opening_ratio / 2);
      }

      translate([0, 0, 3])
        rotate_extrude(angle=360)
          translate([lid_sensor_opening_x_offset, 0, 0])
            circle(r=1.75);

      rotate([0, 0, 180 - 45 / 2])
        translate([lid_sensor_opening_x_offset, 1.5, 2])
          rotate([0, 0, 180])
            cube([container_diameter / 4, 3, 1.75 * 2]);

      rotate([0, 0, 180 - 45 / 2])
        translate([17.5, -1.5, 0])
          cube([container_hole_diameter / 2, 3, 1.75 * 2]);

      // shaft entrance
      translate([0, container_hole_y_offset, -1.501 * 2])
        rotate([0, 180, 0])
          color("Goldenrod") {
            // shaft
            translate([0, 8, -5]) {
              rotate([0, 0, 22.5])
                difference() {
                  cylinder(d=5, h=10);

                  // shaft cutouts
                  translate([1.5, -2.5, -0.1])
                    cube([2, 5, 4.5 + 0.1]);

                  translate([-2 - 1.5, -2.5, -0.1])
                    cube([2, 5, 4.5 + 0.1]);
                }
            }
          }
    }
}

module device() {
  translate([wall_thickness, base_depth + wall_thickness, wall_thickness])
    base_backcover();
  base();

  translate([base_width / 2, base_depth / 2, base_height + base_container_spacing])
    container();

  // center guide
  /*
  translate([base_width / 2, base_depth / 2, base_height + base_container_spacing + container_height])
    cube([container_diameter, 1, 1], center=true);
  translate([base_width / 2, base_depth / 2, base_height + base_container_spacing + container_height])
    cube([1, container_diameter, 1], center=true);
*/

  translate([base_width / 2, base_depth / 2, base_height + base_container_spacing + container_height + lid_overhang_height])
    rotate([0, 0, -(360 / 8) * 1.5])
      rotate([0, 0, 180])
        lid();
}

device();

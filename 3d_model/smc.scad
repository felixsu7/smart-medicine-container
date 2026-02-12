use <./stepper.scad>;
use <./smooth_prim.scad>;

INCHES = 25.4;
EPSILON = 0.001;

$fn = 20;
wall_thickness = 3;
LCD_depth = 4;
LCD_height = 49;
LCD_width = 69;
antenna_diameter = 8;
base_container_spacing = 0.1 * INCHES;
base_depth = 6 * INCHES;
base_height = 2.5 * INCHES;
base_roundedness = 5;
base_top_hole_diameter = 2 * INCHES;
base_width = 6 * INCHES;
base_footing_height = 2.5;
base_footing_diameter = 10;
base_backcover_thickness = 2;
button_diameter = 12;
button_height = 7.4;
compartment_slope_end_r = 22;
compartment_slope_start_r = 25;
container_diameter = 5.5 * INCHES;
container_height = 2 * INCHES;
container_hole_diameter = 1.3 * INCHES;
container_hole_thickness = 1;
container_hole_y_offset = -8; //-0.4 * INCHES;
container_outer_slope_slant = 17.5;
default_hinge_diameter = 2;
default_hinge_height = 3;
default_hinge_thickness = 2;
divider_height = container_height + 0.5;
divider_slant = 4;
divider_thickness = 2;
lid_opening_ratio = 0.35;
lid_overhang_height = 4;
lid_overhang_thickness = 2;
lid_overhang_thickness_slop = 0.1;
lid_diameter = container_diameter / 2 - lid_overhang_thickness - wall_thickness;
lid_sensor_opening_x_offset = container_diameter / 2 * lid_opening_ratio + container_diameter * 0.15;
lid_thickness = 6;
pcb_depth = 80;
pcb_height = 7;
pcb_width = 120;
show_base = false;
show_base_backcover = false;
show_container = true;
show_lid = true;
stepper_diameter = 28;
stepper_height = 50;

// not really, there is no base for example
module button() {
  cylinder(h=button_height + EPSILON, d=button_diameter);
}

module pcb_mount() {
  difference() {
    cube([pcb_width + wall_thickness, pcb_depth + wall_thickness, pcb_height]);
    translate([wall_thickness / 2, wall_thickness / 2, -EPSILON])
      cube([pcb_width, pcb_width, pcb_height + EPSILON * 2]);
  }
}

module base_hinge(hinge_height = default_hinge_height, hinge_diameter = default_hinge_diameter, hinge_thickness = default_hinge_thickness) {

  difference() {
    union() {
      cylinder(h=hinge_height, d=hinge_diameter + hinge_thickness);
      translate([-hinge_diameter, -hinge_diameter, 0])
        cube([hinge_diameter, hinge_diameter, hinge_height]);
    }

    translate([0, 0, -EPSILON])
      cylinder(h=hinge_height + EPSILON * 2, d=hinge_diameter);
  }
}

module base() {
  // weirdass coordinates
  translate([wall_thickness * 2 + base_width / 16, wall_thickness * 2 + base_depth / 3, wall_thickness - EPSILON])
    pcb_mount();

  color("red")
    difference() {
      SmoothCube([base_width, base_depth, base_height], base_roundedness);

      // hollow 
      translate([wall_thickness, wall_thickness, wall_thickness])
        cube([base_width - wall_thickness * 2, base_depth - wall_thickness * 2, base_height - wall_thickness * 2]);

      // hallow front for LCD
      translate([base_width / 2 - LCD_width / 2, -EPSILON, base_height / 2 - LCD_height / 2])
        cube([LCD_width, LCD_depth + EPSILON * 2, LCD_height]);

      // hallow front for top button
      translate([base_width * 0.85, wall_thickness + EPSILON * 25, base_height / 2 + INCHES / 2])
        rotate([90, 0, 0])
          button();

      // hallow front for bottom button
      translate([base_width * 0.85, wall_thickness + EPSILON * 25, base_height / 2 - INCHES / 2])
        rotate([90, 0, 0])
          button();

      // hallow top for connection to container itself 
      translate([base_width / 2, base_depth / 2 + container_hole_y_offset, base_height - wall_thickness - EPSILON])
        cylinder(d=stepper_diameter, h=wall_thickness + EPSILON * 2);

      // hallow top for antenna
      translate([base_width * 0.1, base_depth * 0.9, base_height - wall_thickness - EPSILON])
        cylinder(h=wall_thickness + EPSILON, d=antenna_diameter);

      // TODO what is this
      translate([base_width / 2, base_depth / 2, base_height - 1.5])
        cylinder(h=container_height, d=container_diameter + 0.25);

      //hallow back
      translate([wall_thickness * 3, base_depth - wall_thickness - EPSILON, wall_thickness * 3])
        cube([base_width - wall_thickness * 6, wall_thickness + 0.01, base_height - wall_thickness * 6]);
    }

  //container mounting
  difference() {
    translate([base_width / 2, base_depth / 2, base_height - 1 - 0.1])
      union() {
        cube([container_diameter, 1, 2], center=true);
        cube([1, container_diameter, 2], center=true);
      }

    // hallow for connection to container
    translate([base_width / 2, base_depth / 2 + container_hole_y_offset, base_height - wall_thickness - 0.005])
      cylinder(d=stepper_diameter, h=5.001);
  }

  //footings
  translate([base_width * 0.15, base_depth * 0.15, -2.5 + EPSILON])
    cylinder(h=base_footing_height, d=base_footing_diameter);
  translate([base_width * 0.85, base_depth * 0.15, -2.5 + EPSILON])
    cylinder(h=base_footing_height, d=base_footing_diameter);
  translate([base_width * 0.15, base_depth * 0.85, -2.5 + EPSILON])
    cylinder(h=base_footing_height, d=base_footing_diameter);
  translate([base_width * 0.85, base_depth * 0.85, -2.5 + EPSILON])
    cylinder(h=base_footing_height, d=base_footing_diameter);

  // hinges
  translate([wall_thickness * 2 + default_hinge_diameter / 2, base_depth + default_hinge_diameter - EPSILON, base_height / 2 + default_hinge_height / 2 - default_hinge_height * 3 * 2.1])
  //  not really parametric
  for (i = [0:6]) {
    // blocker for the bottommost
    if (i == 0) {
      translate([0, 0, -default_hinge_height])
        cylinder(h=default_hinge_height / 3, d=default_hinge_diameter + EPSILON);
    }
    translate([0, 0, default_hinge_height * 2.1 * i])
      rotate([180, 0, 90])
        base_hinge();
  }

  // other side hinge
  translate([base_width - wall_thickness * 2 - default_hinge_diameter / 2, base_depth + default_hinge_diameter - EPSILON, base_height / 2 - default_hinge_height / 2 - default_hinge_height * 2.5]) {
    rotate([0, 0, 90])
      base_hinge(hinge_thickness=default_hinge_thickness);

    //blocker
    cylinder(h=default_hinge_height / 3, d=default_hinge_diameter + EPSILON);
  }
}

module base_backcover() {
  // the cover itself
  cube([base_width - wall_thickness * 6, base_backcover_thickness, base_height - wall_thickness * 6]);

  // hinges
  translate([-default_hinge_diameter * 1.5, base_backcover_thickness, base_height / 2 - default_hinge_height * 7.75]) {
    for (i = [0:5])
      translate([base_backcover_thickness / 2, base_backcover_thickness, default_hinge_height * 2.1 * i])
        rotate([180, 0, 180])
          base_hinge();
  }

  // handle kinda
  translate([0, base_backcover_thickness, 0])
    cube([base_backcover_thickness * 4, base_backcover_thickness, base_height - wall_thickness * 6]);

  // hinge on other side
  translate([base_width - wall_thickness * 5 - default_hinge_thickness / 2, base_backcover_thickness * 2, base_height / 2 - base_backcover_thickness * 7])
    base_hinge();

  // handle for it
  translate([base_width - wall_thickness * 6 - base_backcover_thickness * 2, base_backcover_thickness, base_height / 2 - base_backcover_thickness * 8])
    cube([base_backcover_thickness * 2, base_backcover_thickness, base_backcover_thickness * 4]);
}

module stepper_profile() {
  cylinder(d=stepper_diameter, h=stepper_height);

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
        mount_tabs();

    //  translate([-14.6 / 2, -18, -17 + container_height])
    //  cube([14.6, 5.8, 100]);

    translate([0, 0, -0.01])
      cylinder(6.01, 100, 100);

    translate([-14.95 / 2, -container_hole_diameter / 2 - 2, -50])
      cube([14.95, 2, 100]);
  }

  translate([35 / 2, 0, container_height - 1.3])
    cylinder(d=3.5, 1.5);

  translate([-35 / 2, 0, container_height - 1.3])
    cylinder(d=3.5, 1.5);

  translate([0, 0, container_height - 1.299])
    rotate([0, 180, 0])
      stepper_28byj48();
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

    translate([0, container_hole_y_offset, -1])
      cylinder(h=100, d=44);

    // lid overhang entrance
    translate([0, 0, container_height - lid_overhang_height])
      difference() {
        cylinder(lid_overhang_height * 2, container_diameter / 2 - wall_thickness + 0.01, container_diameter / 2 - wall_thickness + 0.01);

        cylinder(lid_overhang_height * 2, container_diameter / 2 - wall_thickness - lid_overhang_thickness, container_diameter / 2 - wall_thickness - lid_overhang_thickness);
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
    /*
    translate([-1.75, -container_hole_diameter / 2 - 10, container_height - wall_thickness - 1.495 * 3])
      cube([1.75 * 2, 10, 3]);
	  */
  }
}

module stepper_cover() {
  difference() {
    cylinder(h=2.5, d=44);
    translate([35 / 2, 0, -0.1])
      cylinder(d=3.5, 3);

    translate([-35 / 2, 0, -0.1])
      cylinder(d=3.5, 3);

    translate([0, 8, -2])
      cylinder(d=9, h=5);
  }
}

module container() {

  color("blue")
    difference() {
      cylinder(container_height, container_diameter / 2, container_diameter / 2);

      // hallow
      translate([0, 0, wall_thickness - EPSILON])
        cylinder(container_height, container_diameter / 2 - wall_thickness, container_diameter / 2 - wall_thickness);

      //hole
      translate([0, container_hole_y_offset, -wall_thickness - EPSILON])
        cylinder(d=stepper_diameter, h=stepper_height);

      // tightening
      difference() {
        translate([0, 0, 0.499]) {
          cube([container_diameter, 1, 2], center=true);
          cube([1, container_diameter, 2], center=true);
        }

        translate([base_width / 2, base_depth / 2, base_height - wall_thickness - 0.005])
          cylinder(d=stepper_diameter, h=stepper_height);
      }
    }

  color("orange")
    translate([0, container_hole_y_offset, EPSILON * 2])
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

      translate([0, 0, 2])
        rotate_extrude(angle=360)
          translate([lid_sensor_opening_x_offset, 0, 0])
            circle(r=1.25);

      rotate([0, 0, 180 - 45 / 2])
        translate([lid_sensor_opening_x_offset, 1.25, 1.25 / 2])
          rotate([0, 0, 180])
            cube([lid_sensor_opening_x_offset / 2, 1.25 * 2, 1.25 * 2]);

      rotate([0, 0, 180 - 45 / 2])
        translate([23.5, -1.25, 0])
          cube([2.5, 2.5, 1.25 * 2]);

      // shaft mount
      translate([0, container_hole_y_offset, -1.5])
        rotate([0, 180, 0])
          // shaft
          translate([0, 8, -5]) {
            rotate([0, 0, 22.5 + 90])
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

module device() {
  if (show_base_backcover) {
    translate([wall_thickness * 3, base_depth - base_backcover_thickness * -1, wall_thickness * 3])
      base_backcover();
  }

  if (show_base) {
    base();
  }

  if (show_container) {
    translate([base_width / 2, base_depth / 2, base_height + base_container_spacing]) {
      rotate([0, 0, 180])
        container();

      translate([0, -container_hole_y_offset, container_height - 2])
        rotate([0, 0, 180])
          stepper_cover();
    }
  }

  if (show_lid) {
    translate([base_width / 2, base_depth / 2, base_height + base_container_spacing + container_height + 0.75])
      rotate([0, 0, -(360 / 8) * 1.5])
        lid();
  }
}

device();

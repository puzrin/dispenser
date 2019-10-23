e = 0.01;
2e = 0.02;

$fn = 128;

pcb_wide_w = 25;
pcb_wide_len = 40;
pcb_narrow_w = 15;
pcb_len = 120;

pcb_color = "Pink";


motor_dia = 15;
motor_len = 20;
motor_flange_len = 1; // shaft flange bearing outer
motor_shaft_len = 7;
motor_shaft_dia = 3;

motor_color = "RoyalBlue";

shaft_coupler_dia = 9;
shaft_coupler_len = 20;
shaft_coupler_offset = motor_len + motor_flange_len;

pusher_path_len = 35;

pusher_rod_len = pusher_path_len + 7;
pusher_rod_dia = 3;

pusher_len = pusher_path_len + 7;
pusher_dia = 11;

pusher_max_offset = motor_len + motor_flange_len + shaft_coupler_len + pusher_path_len + pusher_len;


////////////////////////////////////////////////////////////////////////////////
// Syringe sizes

s_medical_10cc = [
    20.4, // flange_h
    31.6, // flange_w
    16.5, // outer_dia
    14.5, // inner_dia
    75.3, // inner length
    6.5   // pusher head fatness
];

s_pneumo_10cc = [
    22.5, // flange_h
    34.1, // flange_w
    18.5, // outer_dia
    15.5,  // inner_dia
    75.3, // inner length
    2   // pusher head fatness
];

function s_flange_h(data) = data[0];
function s_flange_w(data) = data[1];
function s_outer_dia(data) = data[2];
function s_inner_dia(data) = data[3];
function s_body_len(data) = data[4];
function s_pusher_head(data) = data[5];
function s_axis_z(data) = s_flange_h(data) / 2;

function s_offset(data) = pusher_max_offset - s_body_len(data) + s_pusher_head(data);


////////////////////////////////////////////////////////////////////////////////
// Printed elements

module pusher() {
    //cylinder(pusher_len, d = pusher_dia);
    inner = 4/2 + 0.1;
    outer = pusher_dia/2;
    enforcer_h = 10;
    rounding = 0.5;
    gap_h = 15;
    
    f_ofs = 20;
    f_h = 3;
    
    //color("Red", 0.4)
    difference() {
        rotate_extrude()
        polygon([
            [ inner, 0 ],

            // stopper for metal enforcer insert
            [ inner, enforcer_h ],
            [ inner - 0.4, enforcer_h + 0.4 ],
            [ inner,  enforcer_h + 0.4 + 0.4 ],
        
            [ inner, pusher_len ],
            [ outer - rounding, pusher_len ],
            [ outer, pusher_len - rounding ],

            // Place for soft friction material
            [ outer, f_ofs +f_h + 0.5 ],
            [ outer - 0.5, f_ofs + f_h ],
            [ outer - 0.5, f_ofs ],
            [ outer, f_ofs - 0.5 ],


            [ outer, rounding ],
            [ outer - rounding, 0 ],
        ]);
        
        translate([ 0, 0, pusher_len - gap_h/2 + e ])
        cube([pusher_dia + e, 1, gap_h], center = true);

        rotate([ 0, 0, 90 ])
        translate([ 0, 0, pusher_len - gap_h/2 + e ])
        cube([pusher_dia + e, 1, gap_h], center = true);
    }
}

module motor_support(syringe) {
    space = (s_flange_h(syringe) - motor_dia) / 2;

    difference() {
        translate([ 0, -pcb_wide_w/2, 0 ])
        cube([ motor_len, pcb_wide_w, space * 2 ]);
        
        motor_body(syringe);
    }
}

module syringe_support(syringe) {
    space = (s_flange_h(syringe) - s_outer_dia(syringe)) / 2;
    support_len = 15;
    support_w = 10;

    difference() {
        translate([ 0, -support_w/2, 0 ])
        cube([support_len, support_w, space * 1.5]);
        
        translate([ -e, 0, s_flange_h(syringe)/2 ])
        rotate([0, 90, 0])
        cylinder(support_len + 2e,d = s_outer_dia(syringe));
    }
}

////////////////////////////////////////////////////////////////////////////////
// Virtual elements

module pcb() {
    smoothed_len = (pcb_wide_w - pcb_narrow_w) / 2;
    
    color(pcb_color, 0.5)
    translate([ 0, 0, -2 ])
    linear_extrude(1.6)
    difference() {
        polygon([
            [ 0, pcb_wide_w/2 ],
            [ pcb_wide_len, pcb_wide_w/2 ],
            [ pcb_wide_len + smoothed_len, pcb_narrow_w/2 ],
            [ pcb_len, pcb_narrow_w/2 ],
            [ pcb_len, -pcb_narrow_w/2 ],
            [ pcb_wide_len + smoothed_len, -pcb_narrow_w/2 ],
            [ pcb_wide_len, -pcb_wide_w/2 ],
            [ 0, -pcb_wide_w/2 ]    
        ]);
        
        translate([ 51, 0, 0 ]) {
            ofs = pcb_narrow_w / 2;

            translate([ 0, ofs, 0]) circle(d = 3);
            translate([ 0, -ofs, 0]) circle(d = 3);
            translate([ 6, ofs, 0]) circle(d = 3);
            translate([ 6, -ofs, 0]) circle(d = 3);
        }
    }
}


module motor_body(syringe) {
    z_level = s_axis_z(syringe);

    translate([ 0, 0, z_level])
    rotate([0, 90, 0])
    translate([ 0, 0, -e ])
    cylinder(motor_len + 2e, d = motor_dia);
}

module motor(syringe) {
    z_level = s_axis_z(syringe);

    color(motor_color, 0.6)
    motor_body(syringe);

    translate([ 0, 0, z_level])
    rotate([0, 90, 0]) {
        color(motor_color)
        cylinder(motor_len + motor_shaft_len, d = motor_shaft_dia);
    }
}


module pusher_model(syringe) {
    z_level = s_axis_z(syringe);
    
    translate([ shaft_coupler_offset, 0, z_level])
    rotate([0, 90, 0]) {
        // Coupling
        color("SeaGreen", 0.7)
        cylinder(shaft_coupler_len, d = shaft_coupler_dia);

        // Pusher shaft
        color(motor_color, 0.5)
        translate([ 0, 0, shaft_coupler_len/2 ])
        cylinder(pusher_rod_len + shaft_coupler_len/2, d = pusher_rod_dia);
        
        // Working body
        translate([ 0, 0, shaft_coupler_len + pusher_path_len ])
        pusher();
    }
}

module syringe_body(syringe) {
    h = s_body_len(syringe) + 1;
    z_level = s_axis_z(syringe);

    color("Azure", 0.4)
    translate([
        s_offset(syringe),
        0,
        z_level
    ])
    rotate([0, 90, 0])
    difference() {
        union() {
            cylinder(h, d = s_outer_dia(syringe));
            
            translate([ 0, 0, 0.5 ])
            cube([
                s_flange_h(syringe),
                s_flange_w(syringe),
                1
            ], center = true);
            
            translate([ 0, 0, h - e ]) {
                cylinder(9.5, d = 8);
                cylinder(11.5, d = 4);
            }
        };
        
        translate([ 0, 0, -e ])
        cylinder(h - 1, d = s_inner_dia(syringe));
    }
}

////////////////////////////////////////////////////////////////////////////////

syringe = s_medical_10cc;
//syringe = s_pneumo_10cc;

print = true;

if (print == false) {
    pcb();
    
    translate([ 0, 0, 1 ]) {
        motor(syringe);
        pusher_model(syringe);
        syringe_body(syringe);
    }

    motor_support(syringe);

    translate([ 52, 0, 0])
    syringe_support(syringe);

    translate([ 103, 0, 0])
    syringe_support(syringe);
} else {
    pusher();

    translate([ 10, 0, 0])
    motor_support(syringe);

    translate([ 35, 0, 0])
    syringe_support(syringe);

    translate([ 55, 0, 0])
    syringe_support(syringe);
}
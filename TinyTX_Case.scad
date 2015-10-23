// TinyTX case

DRAWfull				= 0;
DRAWbottom			= 1;
DRAWtop				= 1;
ADDvents				= 1;
ADDsquarecutout		= 0;
ADDroundcutout		= 0;
ADDmountingholes	= 1;

wall_thickness	= 2.0;   // minimum 1.0

//problem with pcb length over 46 needs fixing
pcb_w = 23;   // PCB Width (TinyTX3=23, Tiny328=23)
pcb_l = 41.9; // PCB Length (TinyTX3=41.9, Tiny328=36.3)
batt_w = 17;  // Battery holder width (1xAA=17, 2xAA=32)
batt_l = 58;  // Battery holder length (1/2xAA=58)
batt_h = 16;  // Battery holder height (1/2xAA=16)

// vents
vent_l=15;	 // length of vents on top

// square cutout
square_cutout_x=10;
square_cutout_y=-wall_thickness;
square_cutout_z=7.5;
square_cutout_w=10.0; 
square_cutout_h=5.0;

// round cutout
round_cutout_x=13;
round_cutout_z=13;
round_cutout_r=1.5; 

// mounting holes
hole_r=1.5;	// hole radius
hole_c=3;		//	countersink radius


// internal dimensions
ridge_w = 2;
//pcb_l2 = (pcb_l > 46) ? inside_l+pcb_l-46 : pcb_l;
inside_l = (batt_l >= pcb_l) ? batt_l+2*ridge_w : pcb_l+2*ridge_w+9;
//inside_l = (pcb_l > 46) ? inside_l2+pcb_l-46 : inside_l2;
inside_w = pcb_w+batt_w+2*wall_thickness;
inside_h = batt_h+2*wall_thickness;

casesplit = inside_h/2;  // split case in middle
CASEcolour			="Grey";

// calculate box size
box_l  = inside_l + (wall_thickness * 2);
box_w  = inside_w + (wall_thickness * 2);
box_h  = inside_h;
box_w1 = wall_thickness;
box_w2 = box_w - wall_thickness;
box_l1 = wall_thickness;
box_l2 = box_l - wall_thickness;

// rounded corners
corner_radius = wall_thickness*2;

// DRAW items

DRAWtotal = DRAWfull + DRAWbottom + DRAWtop; // count no of items to draw

// if one draw it centered
if (DRAWtotal == 1) {
    translate(v = [ -box_w/2, -box_l / 2, 0]) {
        if (DRAWfull ) {
           draw_case(0,1);
           translate(v=[0,0,0.2]) draw_case(1,0);
        }
        if (DRAWbottom   ) {
            rotate(a=[0,180,0]) translate(v=[-box_w,0,-box_h]) draw_case(0,1);
        }
        if (DRAWtop) draw_case(1,0);
    }
} else {
// otherwise draw each one next to the other
    if (DRAWfull)   {
        translate(v = [ -box_w-15, 5, box_h]) {
            rotate(a=[0,180,0]) draw_case(1,0);
            rotate(a=[0,180,0]) translate(v=[0,0,0.2]) draw_case(0,1);
        }
    }
    if (DRAWtop) {
        translate(v = [ 5, 5, 0]) draw_case(1,0);
    }
    if (DRAWbottom)    {
        translate(v = [ - 5,  5, box_h])  rotate(a=[0,180,0]) draw_case(0,1);
    }
}

// Modules

module rounded_corner(x,y,rx,ry,cx,cy) {
    translate(v = [x, y, -1]) {
        difference() {
            translate(v=[rx,ry,0])    cube([corner_radius,corner_radius,box_h+2], center=false);
            translate(v=[cx, cy, -1]) cylinder(r=corner_radius, h=box_h+2, $fn=100);
        }
    }
}

module make_square_cutout() {
    // cut out, offset from 0,0
	set_square_cutout(box_w1+square_cutout_x,box_l1+square_cutout_y,square_cutout_z,square_cutout_w,wall_thickness+ridge_w,square_cutout_h+0.1);
}

module make_round_cutout() {
	set_round_cutout(round_cutout_x,round_cutout_z,round_cutout_r);
}

module make_vents() {
    set_vent(20,0,0,1,vent_l,8);
    set_vent(16,0,0,1,vent_l,8);
    set_vent(12,0,0,1,vent_l,8);
    set_vent(8,0,0,1,vent_l,8);

    set_vent(20,box_l-vent_l,0,1,vent_l,8);
    set_vent(16,box_l-vent_l,0,1,vent_l,8);
    set_vent(12,box_l-vent_l,0,1,vent_l,8);
    set_vent(8,box_l-vent_l,0,1,vent_l,8);
}

module make_mounting_holes() {
	color(CASEcolour)    translate(v = [10, box_l-10, box_h-wall_thickness*2+0.1]) cylinder(r1=hole_c, r2=hole_r, h=wall_thickness*2, $fn=100);; 
	color(CASEcolour)    translate(v = [10, 10, box_h-wall_thickness*2+0.1]) cylinder(r1=hole_c, r2=hole_r, h=wall_thickness*2, $fn=100);; 
	color(CASEcolour)    translate(v = [box_w-10, 10, box_h-wall_thickness*2+0.1]) cylinder(r1=hole_c, r2=hole_r, h=wall_thickness*2, $fn=100);; 
	color(CASEcolour)    translate(v = [box_w-10, box_l-10, box_h-wall_thickness*2+0.1]) cylinder(r1=hole_c, r2=hole_r, h=wall_thickness*2, $fn=100);; 
}

module make_case(w,d,h,bt,lt) { // width,depth,height,boxthickness,ridgethickness
    dt=bt+lt;     // box+ridge thickness
    bt2=bt+bt;    // 2x box thickness
    // Substract the shape we have created in the four corners
    color(CASEcolour) difference() {
        cube([w,d,h], center=false);
        if (rounded) {
            rounded_corner(-0.1,d+0.1,0,-corner_radius,corner_radius,-corner_radius);
            rounded_corner(w+0.1,d+0.1,-corner_radius,-corner_radius,-corner_radius,-corner_radius);
            rounded_corner(w+0.1,-0.1,-corner_radius,0,-corner_radius,corner_radius);
            rounded_corner(-0.1,-0.1,0,0,corner_radius,corner_radius);
        }
        // empty inside, but keep a ridge for strength
        translate(v = [bt, bt, dt])   cube([w-bt2, d-bt2, h-(dt*2)], center = false);
        translate(v = [dt,dt,bt])     cube([w-(dt*2), d-(dt*2), h-bt2], center = false);
    }

}

module set_square_cutout(x,y,z,w,l,h) {
    color(CASEcolour)    translate(v = [x+(w/2), y+(l/2), z+(h/2)]) cube([w+0.4, l+0.2, h+0.2], center = true);
}

module set_round_cutout(x,z,r) {
    	rotate(a=[90,0,0]) {
			color(CASEcolour)    translate(v = [x, z, -wall_thickness-0.5]) cylinder(r=r, h=wall_thickness*2, $fn=100);; }
}

module set_vent(x,y,z,w,l,h) {
    color(CASEcolour)    translate(v = [x+(w/2), y+(l/2), z+(h/2)]) cube([w+0.4, l+0.2, h+0.2], center = true);

//round the ends
	color(CASEcolour)    translate(v = [x+(w/2), y+w-1., -0.1]) cylinder(r=w/2+0.2, h=5, $fn=100);;

	color(CASEcolour)    translate(v = [x+(w/2), y+w+l-1, -0.1]) cylinder(r=w/2+0.2, h=5, $fn=100);;

	rotate(a=[90,0,0]) {
		color(CASEcolour)    translate(v = [x+0.5, h, -2.5]) cylinder(r=w/2+0.2, h=3, $fn=100);; }

	rotate(a=[90,0,0]) {
		color(CASEcolour)    translate(v = [x+0.5, h, -box_l-0.5]) cylinder(r=w/2+0.2, h=3, $fn=100);; }

}

module remove_bottom() {
    translate(v = [-1,  -1, casesplit])    cube([100,100, box_h], center = false);
}

module remove_top() {
    translate(v = [-1,  -1, -1])    cube([box_w+2, box_l+2, casesplit+1], center = false);
}

module draw_case(top, bottom) {
    difference() {
        union() {
            // make empty case
            difference() {
                make_case(box_w,box_l,box_h, wall_thickness,ridge_w, rounded=true);
                if (bottom != 0 && top == 0)    remove_top();
                if (bottom == 0 && top != 0)    remove_bottom();
            }
            color(CASEcolour) {
                if (top != 0 ) {
                    translate(v=[box_w1+Cutout_x-6,box_l1,1])  cube([3.0, 4,-1.1]);
                    translate(v=[box_w2-15.8    ,box_l1,1])  cube([3.0, 4,-1.1]);
                }

                if (bottom != 0) {
                    // corner tabs
                    translate(v = [box_w1, box_l1, wall_thickness+3.4]) {
                        cube([ridge_w, 4, box_h-wall_thickness-4], center = false);
                    }

                    translate(v = [box_w1, box_l2-10, wall_thickness+3.4]) {
                          cube([ridge_w, 9, box_h-wall_thickness-4], center = false) ;
                                                 }

                    translate(v = [box_w1, box_l2-(box_l2-pcb_l-wall_thickness*2-4), wall_thickness+8]) {
     	                  cube([ridge_w, box_l2-pcb_l-wall_thickness*2-4, box_h-wall_thickness-9], center = false) ;
						 }

                    translate(v = [box_w2-ridge_w, box_l1, wall_thickness+3.4]) {
                        cube([ridge_w,9, box_h-wall_thickness-4], center = false);
                    }

                 	 translate(v = [box_w2-ridge_w, box_l2-10, wall_thickness+3.4]) {
                        cube([ridge_w,9, box_h-wall_thickness-4], center = false);
                    }

						// inner tabs
                   translate(v = [box_w1+pcb_w-ridge_w, box_l1, box_h/2]) {
                    	cube([ridge_w, 4, box_h/2], center = false);
                    }
                   translate(v = [box_w1+pcb_w-ridge_w, box_l2-(box_l2-pcb_l-wall_thickness*2-4), wall_thickness+8]) {
     	              	cube([ridge_w, box_l2-pcb_l-wall_thickness*2-4, box_h-wall_thickness-9], center = false) ;
						 }

						// inner ridge
                   translate(v = [box_w1+pcb_w-ridge_w, 0, box_h-wall_thickness*2]) {
     	             	cube([ridge_w, box_l, wall_thickness*2], center = false) ;
						 }

                }
            }
        } 

		// add cutouts, vents, mounting holes

		if (ADDsquarecutout == 1) {
			make_square_cutout();
		}

		if (ADDroundcutout == 1) {
       make_round_cutout();
		}

		if (ADDvents == 1) {
       make_vents();
		}

		if (ADDmountingholes == 1) {
       make_mounting_holes();
		}

    }

} 

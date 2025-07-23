$fn=100;

d_tube = 4;



module top()
{
difference()
{
translate([0,-28,60]) cube([180,160,4],center=true);
backpanel();
frontpanel();
translate([0,-80,60])cube([6*d_tube,6*d_tube,6],center=true);
cylinder(r=20,h=1000,center=true);
}
translate([0,-80,60])tube_outlet();
}

//PCB
module pcb()
{
translate([0,90/2+27/2+7,-20]) 
{
    difference()
    {
        cube([148,33,80],center=true);
         translate([-143/2,10,74.5/2]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
    translate([143/2,10,74.5/2]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
    translate([-143/2,10,-74.5/2]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
    translate([143/2,10,-74.5/2]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
    }
}
}



module tube_outlet()
{
difference()
{
    
//cylinder(r=4*d_tube,center=true,h=3);
cube([6*d_tube,6*d_tube,4],center=true);

translate([-1.5*d_tube,-1.5*d_tube,0])
{
for(i=[0:2])
{
    for(j=[0:2])
    {

    translate([i*(d_tube+2),j*(d_tube+2),0]) cylinder(r=d_tube/2, h=20,center=true);
    }
}
}
}
}


module pump_drills()
{
translate([-48/2,0,0])cylinder(r=3.5/2,h=10, center=true);
translate([48/2,0,0]) cylinder(r=3.5/2,h=10, center=true);
cylinder(r=28.5/2,h=10, center=true);
}


module pump_holder_x4()
{
difference()
{
    cube([120,110,4], center=true);
    translate([-30,20,0]) pump_drills();
    translate([-30+60,20,0]) pump_drills();
    
    translate([-30,20-40,0]) pump_drills();
    translate([-30+60,20-40,0]) pump_drills();
    
    cylinder(r=24/2,h=10,center=true);
    translate([60,0,0]) cylinder(r=24/2,h=10,center=true);
    translate([-50,0,0]) cylinder(r=12/2,h=10,center=true);
    
    
}
}

module backplate()
{
    difference()
{
translate([0,50,0]) rotate(a=[90,0,0]) cube([180-8,120,4],center=true);

translate([-80,0,30-9]) rotate(a=[90,0,0])cylinder(r=5.5/2,h=1000,center=true);
translate([-80,0,30-60-9]) rotate(a=[90,0,0])cylinder(r=5.5/2,h=1000,center=true);
translate([80,0,30-9]) rotate(a=[90,0,0])cylinder(r=5.5/2,h=1000,center=true);
translate([80,0,30-60-9]) rotate(a=[90,0,0])cylinder(r=5.5/2,h=1000,center=true);    

translate([-80,0,30+9]) rotate(a=[90,0,0])cylinder(r=5.5/2,h=1000,center=true);
translate([-80,0,30-60+9]) rotate(a=[90,0,0])cylinder(r=5.5/2,h=1000,center=true);
translate([80,0,30+9]) rotate(a=[90,0,0])cylinder(r=5.5/2,h=1000,center=true);
translate([80,0,30-60+9]) rotate(a=[90,0,0])cylinder(r=5.5/2,h=1000,center=true);    

    
translate([0,0,50]) rotate(a=[90,0,0])cylinder(r=12/2,h=1000,center=true);  
translate([0,0,-60]) rotate(a=[90,0,0])cylinder(r=24/2,h=1000,center=true);  

    
translate([-40,0,30]) rotate(a=[90,0,0])cylinder(r=5/2,h=1000,center=true);
translate([40,0,30]) rotate(a=[90,0,0])cylinder(r=5/2,h=1000,center=true);   
    
//PCB mounting holes
translate([-143/2,10,74.5/2]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
translate([143/2,10,74.5/2]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
translate([-143/2,10,-74.5/2]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
translate([143/2,10,-74.5/2]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);


//kabelbinder
translate([35/2,10,0]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
translate([-35/2,10,0]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
translate([20/2,10,40]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
translate([-20/2,10,40]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);    
translate([20/2,10,-30]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);
translate([-20/2,10,-30]) rotate(a=[90,0,0])cylinder(r=3/2,h=100,center=true);    


}
    
}


module backpanel()//BLUE
{


backplate();
translate([-60,-3,0]) rotate(a=[0,90,0]) pump_holder_x4();
translate([60,-3,0]) rotate(a=[0,90,0]) pump_holder_x4();
translate([0,-110,0])backplate();
translate([0,-5,120/2+2]) cube([172,114,4],center=true);//for easier grip to the printing plate
}



module frontpanel()//RED
{
translate([0,-54,0]) rotate(a=[90,0,0]) cube([180,120,4],center=true);
translate([-88,20+(20+13)/2,0]) rotate(a=[0,90,0]) cube([120,144+20+13,4],center=true);
translate([88,20+(+20+13)/2,0]) rotate(a=[0,90,0]) cube([120,144+20+13,4],center=true);
    
difference()
{    
translate([0,92+(20+13)+2,0]) rotate(a=[90,0,0]) cube([180,120,4],center=true);
translate([-80,0,30-9]) rotate(a=[90,0,0])cylinder(r=5/2,h=1000,center=true);
translate([-80,0,30-60-9]) rotate(a=[90,0,0])cylinder(r=5/2,h=1000,center=true);
translate([80,0,30-9]) rotate(a=[90,0,0])cylinder(r=5/2,h=1000,center=true);
translate([80,0,30-60-9]) rotate(a=[90,0,0])cylinder(r=5/2,h=1000,center=true);    

translate([-80,0,30+9]) rotate(a=[90,0,0])cylinder(r=5/2,h=1000,center=true);
translate([-80,0,30-60+9]) rotate(a=[90,0,0])cylinder(r=5/2,h=1000,center=true);
translate([80,0,30+9]) rotate(a=[90,0,0])cylinder(r=5/2,h=1000,center=true);
translate([80,0,30-60+9]) rotate(a=[90,0,0])cylinder(r=5/2,h=1000,center=true);    
    
translate([0,0,-60]) rotate(a=[90,0,0])cylinder(r=20/2,h=1000,center=true);  

}

}



//top();


module blue()
{
difference()
{
color("#0000ff") backpanel();
    
translate([-180/2+7/2,100/2,50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([180/2-7/2,100/2,50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([-180/2+7/2,100/2,-50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([180/2-7/2,100/2,-50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
    
translate([-180/2+7/2,-120/2+20,50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([180/2-7/2,-120/2+20,50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([-180/2+7/2,-120/2+20,-50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([180/2-7/2,-120/2+20,-50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);


//TOF sensor
translate([0,-120/2+3/2-0.2,-57]) cube([40,30,30],center=true);
    
}
}


module red()
{
difference()
{
color("#ff0000") translate([0,-10,0]) difference() 
{
    frontpanel();
    translate([0,50/2-6+(+20+13)/2+1,0]) cube([174,143+20+13+2,144],center=true);//clearance for two part construction
}


translate([120/2,-120/2+3/2-0.2,50])rotate([90,0,0]) cylinder(r=2.2/2,h=7,center=true);
translate([-120/2,-120/2+3/2-0.2,50])rotate([90,0,0]) cylinder(r=2.2/2,h=7,center=true);
translate([120/2,-120/2+3/2-0.2,-50])rotate([90,0,0]) cylinder(r=2.2/2,h=7,center=true);
translate([-120/2,-120/2+3/2-0.2,-50])rotate([90,0,0]) cylinder(r=2.2/2,h=7,center=true);


translate([-180/2+7/2-0.2,100/2+20,50])rotate([0,90,0]) cylinder(r=2.2/2,h=70,center=true);
translate([180/2-7/2+0.2,100/2+20,50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([-180/2+7/2-0.2,100/2+20,-50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([180/2-7/2+0.2,100/2+20,-50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);

translate([-180/2+7/2,-120/2+20,50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([180/2-7/2,-120/2+20,50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([-180/2+7/2,-120/2+20,-50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);
translate([180/2-7/2,-120/2+20,-50])rotate([0,90,0]) cylinder(r=2.2/2,h=7,center=true);

//TOF sensor
translate([19.3/2,-120/2+3/2-0.2,-55])rotate([90,0,0]) cylinder(r=3.2/2,h=17,center=true);
translate([-19.3/2,-120/2+3/2-0.2,-55])rotate([90,0,0]) cylinder(r=3.2/2,h=17,center=true);
translate([0,-120/2+3/2-0.2,-57])rotate([90,0,0]) cube([5,4,17],center=true);
}
}


red();
//translate([0,20,0]) blue();
//pcb();

/*
difference()
{
cube([60,40,2], center=true);
translate([0,0,0]) pump_drills();
}
*/
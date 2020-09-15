'use strict';
delete require.cache[require.resolve('./robot')]; //Delete require() cache
const Robot = require('./robot');

delete require.cache[require.resolve('./modules/lidarx2')]; //Delete require() cache
const Lidar = require('./modules/lidarx2');

//delete require.cache[require.resolve('./modules/lidarLocalisation')]; //Delete require() cache
//const LidarLocalisation = require('./modules/LidarLocalisation');

delete require.cache[require.resolve('./modules/arm')]; //Delete require() cache
const Arm = require('./modules/arm');

const utils = require("../utils")

module.exports = class Robot2020 extends Robot{
    constructor(app) {
        super(app);
        this.name = "Robot Nesnes TDS"
        this.radius = 140;
        this.startPosition = {
            blue:{x:200,y:900,angle:0},
            yellow:{x:2800,y:900,angle:180}
        }
        this.variables = {
            buoyStorageFrontA: { value: 0,  max: 1 },
            buoyStorageFrontB: { value: 0,  max: 1 },
            buoyStorageSideA: { value: 0,  max: 2 },
            buoyStorageSideB: { value: 0,  max: 2 },
            windsocks: { value: 0,  max: 2 }
        }
        this.collisionDistance = this.radius+175;
        this.slowdownDistance = this.collisionDistance+100;
        //this.modules.lidar = new Lidar(app)
        //this.modules.lidarLocalisation = new LidarLocalisation(app)
        this.modules.arm = new Arm(app);
    }

    async init(){
        await super.init();
        if(this.modules.arm && this.modules.robotLink){
            await this.modules.arm.init().catch((e)=>{
                this.modules.arm = null;
            })
        } else this.modules.arm = null;
        this.sendModules();
    }

    async close(){
        await super.close();
        //custom close here
    }

    async activateLighthouse(parameters){
        this.app.logger.log("  -> Activating ligthouse");
        this.addScore(10);
        await utils.sleep(500);
        return true
    }

    async grabStartingBuoys(parameters){
        this.variables.buoyStorageFrontA.value++;
        this.variables.buoyStorageFrontB.value++;
        this.app.map.removeComponent(this.app.map.getComponent("buoyStartingNorth", this.team));
        this.app.map.removeComponent(this.app.map.getComponent("buoyStartingFairwayNorth", this.team));
        await utils.sleep(500);
        return true
    }

    async grabBuoysTop(parameters){
        this.variables.buoyStorageSideA.value+=2;
        this.app.map.removeComponent(this.app.map.getComponent("buoyTop", this.team));
        this.app.map.removeComponent(this.app.map.getComponent("buoyMiddleTop", this.team));
        await utils.sleep(500);
        return true
    }

    async grabBuoysBottom(parameters){
        this.variables.buoyStorageSideB.value+=2;
        this.app.map.removeComponent(this.app.map.getComponent("buoyMiddleBottom", this.team));
        this.app.map.removeComponent(this.app.map.getComponent("buoyBottom", this.team));
        await utils.sleep(500);
        return true
    }

    async grabReaf(parameters){
        this.variables.buoyStorageFrontA.value++;
        this.variables.buoyStorageFrontB.value++;
        await utils.sleep(500);
        return true
    }

    async depositBuoys(parameters){
        this.variables.buoyStorageFrontA.value = 0;
        this.variables.buoyStorageFrontB.value = 0;
        this.variables.buoyStorageSideA.value = 0;
        this.variables.buoyStorageSideB.value = 0;
        this.addScore(20);
        await utils.sleep(500);
        return true
    }

    async setArmPosition(parameters){
        await utils.sleep(500);
        return true;
    }

    async validateWindsock(parameters){
        this.variables.windsocks.value+=1;
        if(this.variables.windsocks.value==1) this.addScore(5);
        if(this.variables.windsocks.value==2) this.addScore(10);
        return true;
    }

}

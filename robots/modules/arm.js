'use strict';
const utils = require("../../utils")

delete require.cache[require.resolve('./robotLink')]; //Delete require() cache
const Robotlink = require('./robotLink');

module.exports = class Arm {
    constructor(app) {
        this.app = app;
        this.address = 6;
        this.link = new Robotlink(app, "Actuators2023");
    }
    
    async init(){
        await this.link.init().catch((e)=>{
            this.link.close();
            this.link = null;
        })
        if(this.link && this.link.connected){
            this.app.logger.log("Arm link OK");
        }
        else{
            this.app.logger.log("==> ERROR: Arm link not connected");
        }
        //this.send();
    }

    getDescription(){
        return {
            functions:{
                setPose4:{
                    name:{ legend:"name", type:"text" },
                    a1:{ legend:"lift", type:"range", min:0, max:360, value:40, step:1 },
                    a2:{ legend:"distrib", type:"range", min:0, max:300, value:150, step:1 },
                    a3:{ legend:"finger1", type:"range", min:0, max:300, value:150, step:1 },
                    a4:{ legend:"finger2", type:"range", min:0, max:300, value:150, step:1 },
                    duration:{ type:"range", min:0, max:1000, value:0, step:1 }
                },
                setServo: { name:{ legend:"name", type:"text" },
                            angle:{ legend:"angle", type:"range", min:0, max:360, value:150, step:1 },
                            duration:{ type:"range", min:0, max:1000, value:0, step:1 }
                },
                setLed: {   brightness:{ legend:"brightness", type:"range", min:0, max:255, value:150, step:1 },
                            color:{ type:"range", min:0, max:255, value:0, step:1 }
                }
            }
        }
    }
    
    async setPose4(params){
        //this.app.logger.log("set pose4");
        return this.setPose(params);
    }

    async setPose(params){
        //this.app.logger.log("set pose");
        if(!("name" in params)) return "ERROR";
        if(!("duration" in params)) params.duration = 0;
        if(!("wait" in params)) params.wait = true;
        let msg = "Z "+params.name+" "+parseInt(""+params.duration);
        if("a1" in params) msg += " "+parseInt(""+params.a1);
        if("a2" in params) msg += " "+parseInt(""+params.a2);
        if("a3" in params) msg += " "+parseInt(""+params.a3);
        if("a4" in params) msg += " "+parseInt(""+params.a4);
        if("a5" in params) msg += " "+parseInt(""+params.a5);
        if("a6" in params) msg += " "+parseInt(""+params.a6);
        if("a7" in params) msg += " "+parseInt(""+params.a7);
        if("a8" in params) msg += " "+parseInt(""+params.a8);
        if("a9" in params) msg += " "+parseInt(""+params.a9);
        if("a10" in params) msg += " "+parseInt(""+params.a10);
        console.log("Arm pose", msg)
        let result = true;
        if(this.link)
            result = await this.link.sendMessage(this.address, msg);
        if(params.wait) await utils.sleep(params.duration);
        return result;
    }

    async setServo(params){
        //this.app.logger.log("servo set");
        let wait = true;
        if(!("duration" in params)) params.duration = 0;
        if(("wait" in params)) wait = params.wait;
        if(!("name" in params)) return "ERROR";
        let msg = "S "+params.name+" "+params.angle+" "+params.duration;
        let result = true;
        if(this.link)
            result =  await this.link.sendMessage(this.address, msg);
        if(wait) await utils.sleep(params.duration);
        return result;
    }

    async setLed(params){
        //this.app.logger.log("set led");
        const ledCount = 12;
        let colors = [];
        if(!("brightness" in params)) params.brightness = 255;
        if("colors" in params && Array.isArray(params.colors)){
            for(let i=0;i<ledCount && i<params.colors.length;i++){
                colors.push(params.colors[i]);
            }
        }
        if("color" in params){
            for(let i=0;i<ledCount;i++){
                colors.push(params.color);
            }
        }
        let msg = "p "+params.brightness;
        for(let i=0;i<ledCount;i++){
            msg += " "+colors[i];
        }
        let result = true;
        if(this.link)
            result =  await this.link.sendMessage(this.address, msg);
        return result;
    }

    async close(){
        if(this.link) await this.link.close()
    }
}
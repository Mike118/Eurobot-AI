'use strict';
delete require.cache[require.resolve('./goals')]; //Delete require() cache
const Goals = require('./goals');

module.exports = class GoalsTest extends Goals{
    constructor(app) {
        super(app);
        this.defaultSpeed=0.4; //m/s 0.4
        this.moveSpeed = 0.4; //m/s 0.4
        this.defaultNearDist=20;//50mm
        this.defaultNearAngle=3;//10°

        let waitForStart = {
            name: "Wait for start",
            condition: ()=>{return true;},
            executionCount: 1,
            actions: [
                {
                    name: "Wait for start",
                    method: "waitForStart"
                },
            ]
        };

        let findAndGrab = (elementList)=>{
            return {
                name: "Find and grab",
                condition: ()=>{
                    let hasArmFree = this.app.robot.variables.armAC.value == "" || this.app.robot.variables.armAB.value == "" || this.app.robot.variables.armBC.value == "";
                    let hasTimeElapsed = this.app.intelligence.currentTime >= 50*1000;
                    let hasTimeLeft = this.app.intelligence.currentTime <= this.app.intelligence.matchDuration-28*1000;
                    let endReached = this.app.robot.variables.endReached.value;
                    return hasArmFree && hasTimeElapsed && hasTimeLeft && !endReached;
                },
                executionCount: 1,
                actions: [
                    {
                        name: "find and grab",
                        method: "findAndGrabCake",
                        parameters:{ plateList: elementList }
                    }
                ],
                onError: [ { name:"pack arms", method:"setArmsPacked", parameters:{}}]
            };
        };

        let rushBrownCakes = ()=>{
            return {
                name: "Rush brown cakes",
                condition: ()=>{
                    let hasArmFree = this.app.robot.variables.armAC.value == "";
                    let isTimeStarting = this.app.intelligence.currentTime <= 2000;
                    let inMatchingStartZone = 900 < this.app.robot.x && this.app.robot.x < 2100;
                    let endReached = this.app.robot.variables.endReached.value;
                    return inMatchingStartZone && hasArmFree && isTimeStarting && !endReached;
                },
                executionCount: 1,
                actions: [
                    {
                        name: "Rush brown cakes",
                        method: "rushBrownFromCenter",
                        parameters:{ }
                    }
                ],
                onError: [ { name:"pack arms", method:"setArmsPacked", parameters:{}}]
            };
        };

        let grabCake = (element=null, iterations=0)=>{
            return {
                name: "Grab cake",
                condition: ()=>{
                    let hasArmFree = this.app.robot.variables.armAC.value == "" || this.app.robot.variables.armAB.value == "" || this.app.robot.variables.armBC.value == "";
                    let hasTimeLeft = this.app.intelligence.currentTime <= this.app.intelligence.matchDuration-28*1000;
                    let endReached = this.app.robot.variables.endReached.value;
                    return hasArmFree && hasTimeLeft && !endReached;
                },
                executionCount: iterations,
                actions: [
                    {
                        name: "Grab cake",
                        method: "grabCake",
                        parameters:{ component: element, speed: this.moveSpeed, nearDist: this.defaultNearDist, nearAngle: this.defaultNearAngle }
                    }
                ],
                onError: [ { name:"pack arms", method:"setArmsPacked", parameters:{}}]
            };
        }
        let depositeCake = (plateTypes, preventBuild=false)=>{
            return {
                name: "Deposit cake",
                condition: ()=>{
                    let hasCakes = this.app.robot.variables.armAC.value != "" || this.app.robot.variables.armAB.value != "" || this.app.robot.variables.armBC.value != "";
                    let hasAllCakes = this.app.robot.variables.armAC.value != "" && this.app.robot.variables.armAB.value != "" && this.app.robot.variables.armBC.value != "";
                    let hasMuchTimeLeft = this.app.intelligence.currentTime <= this.app.intelligence.matchDuration-75*1000;
                    let hasSomeTimeLeft = this.app.intelligence.currentTime <= this.app.intelligence.matchDuration-22*1000;
                    let endReached = this.app.robot.variables.endReached.value;
                    let canDepositAndBuild = hasAllCakes && hasMuchTimeLeft;
                    let canDepositRandom = hasCakes && (preventBuild || !hasMuchTimeLeft) && hasSomeTimeLeft;
                    return !endReached && (canDepositAndBuild || canDepositRandom);
                },
                executionCount: 0,
                actions: [
                    {
                        name: "Deposit cake",
                        method: "depositCake",
                        parameters:{ plateTypes: plateTypes, preventBuild: preventBuild, speed: this.moveSpeed, nearDist: this.defaultNearDist, nearAngle: this.defaultNearAngle }
                    }
                ],
                onError: [ { name:"pack arms", method:"setArmsPacked", parameters:{}}]
            };
        }

        let moveToSpecificEndZone = (plateTypes)=> {
            return{
                name: "Move to specific End",
                condition: ()=>{
                    let isEnd = (this.app.intelligence.currentTime >= this.app.intelligence.matchDuration-15*1000);//9
                    let endReached = this.app.robot.variables.endReached.value;
                    return isEnd && !endReached;
                },
                executionCount: 1,
                actions: [
                    { name: "Return to end zone", method: "returnToSpecificEndZone", parameters:{ plateTypes:plateTypes } },
                    { name: "Store in variable", method: "setVariable", parameters:{ name:"endReached", value:1 } },
                ],
                //onError: [ { name:"Move to new end zone", method:"returnToEndZone", parameters:{ignoreSelected:true}}]
            }
        };

        let moveToEndZone = {
            name: "Move to End",
            condition: ()=>{
                let isEnd = (this.app.intelligence.currentTime >= this.app.intelligence.matchDuration-15*1000);//9
                let endReached = this.app.robot.variables.endReached.value;
                return isEnd && !endReached;
            },
            executionCount: 0,
            actions: [
                { name: "Return to end zone", method: "returnToEndZone", parameters:{ } },
                { name: "Store in variable", method: "setVariable", parameters:{ name:"endReached", value:1 } },
            ],
            onError: [ { name:"Move to new end zone", method:"returnToEndZone", parameters:{ignoreSelected:true}}]
        };

        let moveToEndZoneUpdated = {
            name: "Move to updated End",
            condition: ()=>{
                let isEnd = (this.app.intelligence.currentTime >= this.app.intelligence.matchDuration-15*1000);//9
                let endReached = this.app.robot.variables.endReached.value;
                if(!isEnd || !endReached) return false;

                // Detect if end zone changed
                let zoneList = [];
                let zoneTypes = ["plateProtected", "plateMiddleTop", "plateMiddleBottom", "plateBottomSide", "plateBottom"];
                for(let type of zoneTypes) {
                    zoneList.push(...this.app.map.getComponentList(type, this.app.robot.team));
                }
                let actualZoneValue = 0;
                let maxZoneValue = 0;
                let maxZone = null;
                for(let zone of zoneList){
                    if(zone.cakes) continue;
                    let zoneValue = 0;
                    if(zone.endZone) zoneValue = zone.endZone;
                    if(zone.name == this.app.robot.variables.endZone.value) actualZoneValue = zoneValue;
                    if(maxZoneValue<zoneValue){
                        maxZoneValue = zoneValue;
                        maxZone = zone;
                    }
                }
                let endZoneChanged = actualZoneValue != maxZoneValue
                                     && maxZone.name != this.app.robot.variables.endZone.value;

                return isEnd && endZoneChanged;
            },
            executionCount: 0,
            actions: [
                { name: "Return to end zone", method: "returnToEndZone", parameters:{ } },
                // Store End reached
                { name: "Store in variable", method: "setVariable", parameters:{ name:"endReached", value:1 } },
            ],
            onError: [ { name:"Move to new end zone", method:"returnToEndZone", parameters:{ignoreSelected:true}}]
        };

        let dropCherries = {
            name: "Drop cherries",
            condition: ()=>{
                let teamColor = this.app.robot.team;
                let protectedPlateHasCake = true;
                let component = this.app.map.getComponent("plateProtected", teamColor);
                if(component === null){
                    this.app.logger.log("  -> Component not found ");
                } else if(!component.hasCake) {
                   protectedPlateHasCake = false;
                }
                let hasCherries = this.app.robot.variables.cherryABF.value > 0 || this.app.robot.variables.cherryBCF.value > 0;
                let hasSomeTimeLeft = this.app.intelligence.currentTime <= this.app.intelligence.matchDuration-15*1000;
                let endReached = this.app.robot.variables.endReached.value;
                return !endReached && ! protectedPlateHasCake && hasCherries && hasSomeTimeLeft;
            },
            executionCount: 5,
            actions: [
                {
                    name: "Move to cherries",
                    method: "moveToComponent",
                    parameters:{ component: "experiment", speed: this.moveSpeed, nearDist: this.defaultNearDist, nearAngle: this.defaultNearAngle }
                },
                { name: "Drop cherries", method: "dropCherries", parameters:{}},
                { name: "Rest", method: "travelPosition", parameters:{}}
            ]
        };

        let getCherriesTop = {
            name: "Get cherries top",
            condition: ()=>{
                let hasSomeTimeLeft = this.app.intelligence.currentTime <= this.app.intelligence.matchDuration-15*1000;
                let endReached = this.app.robot.variables.endReached.value;
                return !endReached && hasSomeTimeLeft;
            },
            executionCount: 1,
            actions: [
                { name: "Get cherries Top", method: "getCherriesTop", parameters:{}},
                { name: "Rest", method: "travelPosition", parameters:{}}
            ]
        };

        let dropCherriesStart = {
            name: "Start by drop cherries",
            condition: ()=>{
                let hasCherries = this.app.robot.variables.cherryABF.value > 0 || this.app.robot.variables.cherryBCF.value > 0;
                let inMatchingStartZone = this.app.robot.x < 450;
                let isTimeStarting = this.app.intelligence.currentTime <= 2000;
                return isTimeStarting && hasCherries && inMatchingStartZone;
            },
            executionCount: 1,
            actions: [
                { name: "Drop cherries", method: "dropCherries", parameters:{}},
                { name: "Rest", method: "travelPosition", parameters:{}}
            ]
        };

        let moveTest = {
            name: "Move test",
            condition: ()=>{
                let endReached = this.app.robot.variables.endReached.value;
                return !endReached;
            },
            executionCount: 10,
            actions: [
                {
                    name: "Move",
                    method: "moveToComponent",
                    parameters:{ component: "plateProtected", color:"green", speed: this.moveSpeed, nearDist: this.defaultNearDist, nearAngle: this.defaultNearAngle }
                },
                {
                    name: "Move",
                    method: "moveToComponent",
                    parameters:{ component: "plateProtected", color:"blue", speed: this.moveSpeed, nearDist: this.defaultNearDist, nearAngle: this.defaultNearAngle }
                }
            ]
        };

        this.list = [
		    //moveTest,
            waitForStart,
            rushBrownCakes(),
            dropCherriesStart,
            grabCake(null, 1),
            grabCake(null, 1),
			dropCherries,
            depositeCake(["plateProtected"], true),
            grabCake(null, 1),
            grabCake(null, 1),
            depositeCake(["plateMiddleTop"], true),
            //findAndGrab(["plateMiddleTop", "plateMiddleBottom", "plateBottom"]),
            //Deposit random
            //depositeCake(["plateMiddleTop", "plateMiddleBottom", "plateBottom", "plateBottomSide"], true), // not "plateProtected"
            moveToSpecificEndZone(["plateMiddleBottom"]),
            //moveToEndZone,
            //moveToEndZone,
            //moveToEndZoneUpdated
        ]
    }
}

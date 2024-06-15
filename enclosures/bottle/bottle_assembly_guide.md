# Horizon / Argos 500ml plastic bottle enclosure assembly guide #
## Version 2.2 | June 2024
![arribada_bottle_tracked](https://github.com/arribada/horizon/assets/6997400/358a3fd7-2af0-4aef-a012-f12eff526f08)

## About ##
This repository contains CNC millable and 3D printable files to prepare a 500ml plastic bottle enclosure suitable for tracking everyday 500ml plastic drinks bottles.

Selective Laser Sintering (SLS) 3D printable enclosure STEP files can be [downloaded from the enclosure repository](https://github.com/arribada/horizon/tree/master/enclosures/bottle). The SLS enclosure fits an Horizon ARTIC R2 board and satellite transmitter in the top cavity, connected to a lithium polymer / lithium-ion battery seated in the bottom cavity. The two halves are connected together with an o-ring preventing water ingress, with two foam cavities providing additional buoyancy. 3D printed inserts enable the components to be encapsulated with epoxy / potting solution to protect them from water ingress and saltwater when deployed in the open ocean.

An alternative method to using SLS is to print using reclaimed or recycled nylon on a suitable 3D printer, with reclaimed nylon sourced from discarded or abandoned fishing nets suitable. Waterproofing is achieve through the use of potting solutions to protect internal electronics and batteries. We recommend exploring the use of reclaimed nylon as a sustainable alternative to SLS printing.

**Note** - it will take 72 hours to prepare a bottle due to the curing time necessary when potting the antenna and electronics.

## Licencing ##
All hardware designs, files, assets and schematics in this repository are licenced under CERN OHL v1.2. Documentation is licenced under GPLv3.

## Assembled enclosure views  ##
**[Figure 1 - below]** The horizon board should be positioned with the GNSS ceramic antenna facing upwards. The ARTIC R2 transmitter is connected via 8mm brass spacers below. If using the 3D printed insert, the two boards can be placed inside and secured with hot glue ready for potting. The insert will position the boards correctly for optimal positioning.

The antenna is connected via an SMA to u.fl connector, with the bulkhead of the SMA connector screwed into the antenna opening. The base of the SMA should also be potted to prevent corrosion.

![bottle5](https://github.com/arribada/horizon/assets/6997400/159d4854-ecab-4b31-bdab-505fdefb736d)

**[Figure 2 - below]** The two SLS bottle enclosure parts (top and bottom) are secured together using M3 x 10 bolts and nylong locking nuts. There is an optional window that can be milled out and potted with a clear epoxy (i.e MG Chemical Waterclear) to aid viewing the LED status of the Horizon board inside once closed.

![bottle2](https://github.com/arribada/horizon/assets/6997400/f9fbbe96-8f3c-45e1-9c30-cc31879d9c79)

**[Figure 3 - below]** An alternative view looking up at the top cavity with the Horizon board (blue) and the ARTIC R2 transmitter (green) visible. Note the positioning of the magnetic reed switch. There is an X embossed onto the wall of the enclosure identifying its position once the bottle enclosure has been sealed.

![bottle4](https://github.com/arribada/horizon/assets/6997400/8f522df7-7b3c-4172-a88b-45fbd144230d)

**[Figure 4 - below]** A transparent view of the top cavity. Note the JST power connector on the left hand side. Before installing the Horizon board, connect a JST cable prior to potting the cavity (it's easy to sometimes forget).

![bottle3](https://github.com/arribada/horizon/assets/6997400/8ec906a1-bcc2-4e13-8de8-ab035811e5e8)

**[Figure 5 - below]** A transparent view of both cavities with a 2-cell lithium-ion battery (grey) positioned in the bottom cavity. Additional foam inserts can be placed in the left and right cavities to provide buoyancy aid and to protect against any future water ingress filling the cavities with water.

![bottle1](https://github.com/arribada/horizon/assets/6997400/56dadb6f-c695-408e-9458-28840e7ae710)

---

## Assembly Guide ##

This guide assumes you have pre-configured your Horizon board and have programmed a configuration to support your deployment. If you have ordered pre-printed parts from Arribada and have the bottle assembly kit to hand you will be able to complete a full assembly after purchasing potting compound (not included in the assembly kit). Consult the BOM list for a full list of provide / supplementary parts. If you intend to print your own parts, [review the CAD repository](https://github.com/arribada/horizon/tree/master/enclosures/bottle) too.

## Preparing the enclosure for assembly and installing the antenna ##

**Stage 1** | Select the top enclosure piece. Next, add two to three pea size amounts of synthetic silicone grease or o-ring lubricant to the o-ring and apply it evenly until covered. Position the o-ring in the o-ring cavity of the enclosure piece.

**Stage 2** | Position and screw the ARTIC R2 satellite transmitter board into the Horizon board with the GNSS ceramic antenna pointing upwards. Use 8mm brass or stainless steel spacers and M2 x 4mm screws. Ensure that the picoblade cable assembly is also positioned well and that there is no strain on the cables.

**Stage 3** | Screw the SMA antenna cable into the antenna hole and using hot glue, add a layer of glue around the base of the SMA adapter inside the enclosure to seal it. The reason for this is to guarantee a tight seal as we will pour potting solution into the top of the antenna cavity in the next stage.

**Stage 4** | Prepare 10ml of potting solution if assembling a single bottle. We recommend using MG Chemicals 832-WC. After leaving the 832-WC epoxy to de-gas for 15 minuites, poor it slowly into the antenna cavity using a syringe. If drying at room temperature (21C) you should leave the epoxy to cure for 24 hours - 48 hours. It's necessary to first install the antenna as the enclosure will be positioned upside down whilst completing the installation of the electronics and battery and the antenna mounting hole should be sealed to prevent potting solution from escaping.

## Installing the Horizon and Argos ARTIC R2 satellite transmitter boards ##

**Stage 2** | Temporarily place the board assembly into the 3D printed insert and using hot glue, secure the 3D printed insert in place by attaching it to the SLS enclosure walls, ensuring that it is flush and running straight in alignment with the enclosure's walls. Note - you only need a small amount of glue to temporarily position the 3D insert, as a thin layer of epoxy will be added later on to permanently secure the board.

 **Stage 3** | Using a magnet, hold it next to the reed switch until you see a white LED flashing, indicating that the board has booted. Observe the board as it continues the boot phase. You will see a green LED flash next, indicating that the board has changed to an operational state. When you see a light blue LED flashing the board is searching for a GPS signal. Move the enclosure with the board and battery still connected to an area where a GPS signal can be aquired, or hold the board outside with a clear line of sight to the sky until you see a green LED flash - this lets you know that a fix was successful. The board will not continue and will transmit an Argos satellite message every 60 seconds. You will see a purple LED on the Horizon board when transmitting. If all phases pass, you are ready to encapsulate the board assembly. Turn the board off by holding the magent next to the reed switch again until you see a fast flashing white LED.

 **Stage 4** | Prepare 90ml of potting solution. We recommend using MG Chemicals 832-WC. After following the potting solution's preparation guidance, leave the potting solution to de-gas. Whilst de-gassing, use a small amount of hot glue to position the assembled boards inside the 3D printed insert. The simplest way is to to apply a line of glue to the based of the ARTIC R2 transmitter as indicated below. 

 **Stage 5** | Pour the potting solution into the 3D insert cavity to encapsulate the electronics. Ensure you do this slowly and evenly, making sure to not pore too fast and introduce air bubbles. Continue until the top of the board assembly is completed covered with epoxy. Leave to cure for a minimum of 48 hours as 21C. If your room temperature is cooler, test that the potting solution has cured after 48 hours by touching the surface. It should be completely solid and not tacky or compressible. 



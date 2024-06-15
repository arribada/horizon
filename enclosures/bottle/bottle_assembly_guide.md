# Horizon / Argos 500ml plastic bottle enclosure assembly guide #
## Version 2.2 | June 2024
![arribada_bottle_tracked](https://github.com/arribada/horizon/assets/6997400/358a3fd7-2af0-4aef-a012-f12eff526f08)

## About ##
This repository contains CNC millable and 3D printable files to prepare a 500ml plastic bottle enclosure suitable for tracking everyday 500ml plastic drinks bottles.

The SLS enclosure fits an Horizon ARTIC R2 board and satellite transmitter in the top cavity, connected to a lithium polymer / lithium-ion battery seated in the bottom cavity. The two halves are connected together with an o-ring preventing water ingress, with two foam caviities providing additional buoyancy. 3D printed inserts enable the components to be encapsulated with epoxy / potting solution to protect them from water ingress and saltwater when deployed in the open ocean.

## Licencing ##
All hardware designs, files, assets and schematics in this repository are licenced under CERN OHL v1.2. Documentation is licenced under GPLv3.

## Assembled enclosure views  ##
[Figure 1] The horizon board should be positioned with the GNSS ceramic antenna facing upwards. The ARTIC R2 transmitter is connected via 8mm brass spacers below. If using the 3D printed insert, the two boards can be placed inside and secured with hot glue ready for potting. The insert will position the boards correctly for optimal positioning.

The antenna is connected via an SMA to u.fl connector, with the bulkhead of the SMA connector screwed into the antenna opening. The base of the SMA should also be potted to prevent corrosion.

![bottle5](https://github.com/arribada/horizon/assets/6997400/159d4854-ecab-4b31-bdab-505fdefb736d)

[Figure 2] The two SLS bottle enclosure parts (top and bottom) are secured together using M3 x 10 bolts and nylong locking nuts. There is an optional window that can be milled out and potted with a clear epoxy (i.e MG Chemical Waterclear) to aid viewing the LED status of the Horizon board inside once closed.

![bottle2](https://github.com/arribada/horizon/assets/6997400/f9fbbe96-8f3c-45e1-9c30-cc31879d9c79)

[Figure 3] An alternative view looking up at the top cavity with the Horizon board (blue) and the ARTIC R2 transmitter (green) visible. Note the positioning of the magnetic reed switch. There is an X embossed onto the wall of the enclosure identifying its position once the bottle enclosure has been sealed.

![bottle4](https://github.com/arribada/horizon/assets/6997400/8f522df7-7b3c-4172-a88b-45fbd144230d)

[Figure 4] A transparent view of the top cavity. Note the JST power connector on the left hand side. Before installing the Horizon board, connect a JST cable prior to potting the cavity (it's easy to sometimes forget).

![bottle3](https://github.com/arribada/horizon/assets/6997400/8ec906a1-bcc2-4e13-8de8-ab035811e5e8)

[Figure 5] A transparent view of both cavities with a 2-cell lithium-ion battery (grey) positioned in the bottom cavity. Additional foam inserts can be placed in the left and right cavities to provide buoyancy aid and to protect against any future water ingress filling the cavities with water.

![bottle1](https://github.com/arribada/horizon/assets/6997400/56dadb6f-c695-408e-9458-28840e7ae710)







## Purpose ##
The oceanic manta ray active vacuum tag was developed to provide a non-invasive means of attaching biologging instruments to oceanic manta rays by means of utilising a vaccum cup and active suction to attach biologging payloads to the manta's skin. It is primarily intended for attachment in-water by scuba divers, although a pole-based attachment mechanism is under development. Typical use cases involve attaching GPS/GNSS positioning instruments, accelerometers and other biologging apparatus. 

The tag has been designed to fit Arribada's open source [Horizon ARTIC R2 Argos satellite / GNSS transmitter](https://arribada.org/horizon-gps-tracking/) to provide satellite positioning for recovery when on the surface. Other payloads, such as accelerometers, of optional VHF pingers can be fitted to either side of the tag's cylindrical mounting holes. 

The design files in this repository contain all of the components and necessary to build the physical tag. You are free to select your own electronics / payloads, or to use the Horizon transmitter which fits the tag natively.

What the hell is it?
====================

Lsd is a fast and lightweight multi-language multi-level persistant message queue library and smart [Cocained](https://github.com/Kobolog/cocaine) balancer built on top of ZeroMQ transport.

Notable features:

* Sending messages to [Cocained](https://github.com/Kobolog/cocaine) cloud in simple and persistant manner (no messages lost, yeah!).
* Self-balancing messages to [Cocained](https://github.com/Kobolog/cocaine) workers depending on your specification (fair balancing / worker load-balancing).   
* Different levels of persistance: ram, hdd (eblob), distributed storage (mongo, elliptics, etc).
* Works even if uplink to cloud is lost or non-exisatant, Lsd will deliver messages based on your timeout specifications when uplink is established.
* Automatic discovery of cloud nodes based on host info collected from multicast address or http route.
* Allows aggregation of [Cocained](https://github.com/Kobolog/cocaine) logs information that can be used virtually for any kind of statistics or debugging routines.

At the moment, Lsd supports the following languages and specifications:

* C++
* Python
* Perl
* Node-js

Please note
====================
This project is under heavy development and is being updated every day.
Check back with us soon!
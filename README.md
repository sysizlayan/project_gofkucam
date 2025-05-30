# Welcome to Project GofkuCam
Hi! I am Yiğit, a robotics engineer.
I have two cats, a mother(Gofret) and her son(Haku). The repository is named after them.
I have problem with the son, stealing her motrher's food. To prevent that, I decided to develop something.
The idea simply is to have camera looking at the automatic feeder, observing the cats and if the son arrives close one of the lids vice-versa. I will also use this repo as a development diary.

Features I have in mind:

1) Open/close the lid of the bowl, observing the cats
* Software and its execution platform for cat observations
  * Requires an observation system, an actuator, a communication interface
  * Depth estimation from a mono-camera, cat detection+tracking (both probably be neural, I need to learn how to handle neural models in C++)
  * I wonder if I can fit these in a Raspberry, otherwise Orin or a cheap 2nd hand computer
  * Of course multi-threading or similar (I need to choose and execution framework, maybe ROS,  QP, or QT)
  * It should all be async
* IoT platform for the lid actuator, read the weigth sensors, communication protocol to deliver this information
  * Most probably RC servomotor based actuation (ESP32, MQTT let's check aliexpress for a good devkit if any, preferably with a screen)
2) Log how much, when the cats are eating
* Requires some weight sensors, database and a GUI to observe
* Load cells and good mechanical design to place them under the bowl (maybe cheap food scales)
  * Their integration with ESP32
* Database to log this information
  * I only know MongoDB :D
* GUI
  * For GUI most probably QT
3) A good design for not disurbing the cats with the design
* Is there such a thing like CI/CX?(cat interface, cat experience :P)
4) Regulate the amount to make the son to lose some weight
* Requires a veterinarian to advise me :D (and some behavioural infrastructure to implement this advise)
---
Repo/Development requirements:
1) Develop integration, main release
2) Docker or similar migth be a good idea
3) For observation part, I started with CMake but bazel also might be a good idea.

## Folder structure
* extern: External libraries like libtorch
* resouces: neural weights, videos for testing etc.

### Changelog:
30.05.2025 - Camera connection, frame acquisition and the QP framework active object framework is working. I will continue with choosing neural models.

My laptop is not that good, developing in the old desktop I have migth be a good idea but I do not want to install ubuntu there. Can I connect to the WSL remotely for development? Let's check.

27.05.2025 - Let's start :) I started with the observation system, using the Tapo camera I had. Execution will be based on Quantum Leaps' QP framework, written in C++.
23.05.2025 - Vet visit, advising Haku to lose some weight

## Disclaimer
This is a personal project but open source, so I am not paying too much attention on the licence(s) (yet) since it is not distributed commercially.

I will definitely use open-source software but if you come across with an issue, write to me through sysizlayan@gmail.com, so that I can check and fix it.
# PhotonVHealth - Solar Panel Efficiency Monitoring

---

## NOTE: THIS README IS OUTDATED WHILE DEVELOPMENT IS IN PROGRESS, DON'T TAKE ANY INFORMATION FOUND HERE FOR GRANTED

---

Welcome to PhotonVHealth for solar panel efficiency monitoring, especially useful for DIY project users. My monitor makes sure external factors such as: 

* Dust Accumulation

* Sudden shading

* Overheating

don't cause sudden losses in the efficiency of the panel(s) while going unnoticed.

---

## Components Used:

* **LM35:** for temperature

* **KY-018 LDR:** for light

* **INA219:** for power, current & voltage produced

* **HC-06:** for bluetooth connection (to be replaced with ESP32 insha'Alah)

* **Arduino UNO:** the brain basically (to be replaced with ESP32 insha'Allah)

* **A 12V 3 Watt Panel:** for testing  
<img src = "images/panel.jpeg" height = 15% width = 15%/>

* **Soldering iron + flux:** for soldering the wires to the panel & enclosure work

* **Jumper wires, resistors and breadboard of course**

---

## Future Improvements (Insha'Allah to begin working on somewhere round April 4th):

* Replacing **Arduino UNO & HC-06** with **ESP32**

* Replacing **LM35** with **DS18B20 TO-92 Temp Sensor**

* Usage of a better enclosure

* Building a dashboard website to monitor trends such as power produced over time and more, renew baseline from there & set the max panel power output (Say user's panel produces 3 Watt, user enters 3 in the dashboard website as the max) from there too

---

## LICENSING

This project is licensed under the GNU GPL v3 license, check the **[license file](LICENSE)** for details.

## Author:
**Saif Kayyali**

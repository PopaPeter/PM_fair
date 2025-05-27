# PM_fair
# Proiect Arduino: Dispozitiv Automatizat de Zdrobire a Dozelor din Aluminiu

## Descriere

Acest proiect are ca scop construirea unui dispozitiv automatizat pentru zdrobirea dozelor goale de aluminiu, contribuind la reciclare si reducerea volumului deseurilor. Utilizatorul plaseaza doza intr-un compartiment dedicat si porneste procesul printr-un buton. Dispozitivul foloseste un piston electric de 12V controlat de un driver L298N, cu feedback de la un senzor de curent ACS712 si afisare in timp real pe un LCD 16x2 cu I2C.

---

## Componente utilizate

| Componenta                 | Rol                                                      |
|---------------------------|-----------------------------------------------------------|
| Arduino Uno               | Unitatea centrala de control                              |
| Piston electric 12V (900N)| Zdrobeste dozele de aluminiu                              |
| Driver L298N              | Controleaza directia pistonului (IN1/IN2)                 |
| Senzor curent ACS712 (20A)| Masoara curentul absorbit de piston                       |
| LCD 1602 cu I2C           | Afiseaza curentul (A) si puterea (W)                      |
| Sursa 12V/180W            | Alimenteaza pistonul prin L298N                           |
| 2 butoane de control      | Pornire/Oprire si Schimbare directie piston               |
| LED-uri                   | Indica starea sistemului (rosu: oprit, verde: activ)      |

---

## Functionalitate

- Control Piston:
  - D2 si D3: Comanda directia prin L298N (IN1/IN2).
  - Butoane: D8 (ON/OFF) si D9 (sens).
  - LED-uri: Indica starea activa sau oprita a pistonului.

- Monitorizare Curent si Putere:
  - ACS712 conectat pe pinul A0 masoara curentul.
  - Tensiune de alimentare considerata: 10.65V (masurata).
  - Puterea se calculeaza: P = I × 10.65.

- Afisare pe LCD:
  - Linie 1: Afiseaza curentul si puterea (ex: I:0.55A P:5.9W).
  - Linie 2: Afiseaza starea sistemului: Stare: ACTIV/OPRIT.

- Debounce butoane:
  - Implementat in software, folosind Timer1 in CTC mode.
  - Citire stabila la fiecare 1ms, cu filtrare 20ms.

---

## Diagrama conexiunilor

```text
                +-----------------------------+
                |         Arduino UNO         |
                |                             |
                |  A0  <--- ACS712 OUT        |
                |  D2  ---> L298N IN2         |
                |  D3  ---> L298N IN1         |
                |  D8  <--- Buton ON/OFF      |
                |  D9  <--- Buton DIRECTIE    |
                |  D12 ---> LED Verde         |
                |  D13 ---> LED Rosu          |
                |  A4  ---> I2C SDA (LCD)     |
                |  A5  ---> I2C SCL (LCD)     |
                +-----------------------------+
```

---

## Mediu de dezvoltare

- PlatformIO + Visual Studio Code
- Limbaj: C
- Toolchain: AVR-GCC pentru ATmega328P
- Librarii:
  - lcd.h, lcd.cpp – Interfata I2C pentru LCD 1602
  - twi.h, twi.cpp – Suport pentru I2C

---

## Algoritmi implementati

- Control directie motor cu buton tip toggle
- Debounce software pe Timer1 la 1ms
- Masurare curent si calcul putere
- Afisare dinamica LCD in 2 linii
- Indicatie stare prin LED-uri

---

## Calibrare si validare

- Senzorul ACS712 calibrat la 2.5V = 0A, 100mV/A (varianta 20A).
- Validare efectuata cu multimetru in serie si sarcina de test cunoscuta.
- Tensiune reala alimentare masurata: 10.65V.

---

## Concluzie

Proiectul demonstreaza o aplicare practica a notiunilor de microcontrolere si electronica digitala, oferind un sistem functional pentru reciclarea eficienta a dozelor de aluminiu cu feedback energetic in timp real.

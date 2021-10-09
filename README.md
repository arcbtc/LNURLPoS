# LNURLPoS
## Cheap, offline(!), DIY bitcoin lightning-network PoS

![lnurlpos](https://user-images.githubusercontent.com/33088785/134943216-1a9f3ab6-09da-4e15-b16a-2c2f8bc918da.png)


Lightning-network uses hot wallets, real-world payments are made from phones. The burden of connectivity can be taken away from the point-of-sale and given to the phone.


![image](https://user-images.githubusercontent.com/33088785/134652952-cf5c95ac-afc2-4175-8d09-a983c3f066bc.png)

<i>For a traditional PoS experience see my <a href="https://github.com/arcbtc/LNPoS">LNPoS project</a>.</i>

LNURLPoS uses the LNURL-pay protocol. LNURL-pay allows your lightning-wallet to make a secure request to a server to get a lightning-network invoice. So instead of scanning a massive ugly lightning-network invoice QR, you can scan a lovely little LNURL QR (if you <a href="https://lnurl.fiatjaf.com/codec/">decode</a> an LNURL you'll see its just a URL).  

![image](https://user-images.githubusercontent.com/33088785/134657379-bd9e18f0-0289-498b-a1fc-c084c60e64e3.png)

For online stuff I suppose massive QR codes are not an issue, but when fiddling with hardware devices they are. LNURLPoS using the LNURL-pay protocol, it can use a smaller screen for displaying the QR.

## Setup workflow

* LNURLPoS server set up and register PoS in a few clicks on <a href="https://github.com/lnbits/lnbits/">LNbits</a> using the <a href="https://github.com/lnbits/lnbits/tree/master/lnbits/extensions/lnurlpos">LNURLPoS</a> extension 
* Copy credentials (including a secret key) from server to the physical LNURLPoS device

## Payment workflow

* Merchant enters amount into LNURLPoS device
* LNURL is generated in device and displayed for scanning (LNURL includes a unique pin encrypted using the secret key shared with the server)
* Customer scans and pays
* When the payment has cleared the customer is sent the decrypted unique pin
* Merchant can compare and verify using the same pin displayed on the lNURLPoS

### Credit/props

Stepan Snigerev for creating beautiful crypto and LNURL encoding functions.

Fiatjafs incredible <a href="https://github.com/lnbits/lnbits/tree/master/lnbits/extensions/offlineshop">OfflineShop</a> extension. LNURLPoS is the same concept, but can run at scale, and is dependent on a device. 

<a href="https://www.bleskomat.com/">Belskomat</a> for pironeering the idea of a shared secret for the microcontroller to encrypt data with. 

# ⚡⚡⚡⚡⚡⚡ LNURLPoS Tutorial ⚡⚡⚡⚡⚡⚡

<a target="_blank" href="https://www.youtube.com/watch?v=ofCv2cHZ5b0"><img src="https://user-images.githubusercontent.com/33088785/136672544-658d7fdc-2ed4-4442-9536-eaa3eeaf613a.png"></a>
  
## Hardware needed

* <a href="https://www.aliexpress.com/item/33048962331.html">Lilygo TTGO T-Display</a>
* Keypad membrane (these <a href="https://www.aliexpress.com/item/32812109541.html">big</a> ones are easy to find, these <a href="https://www.aliexpress.com/item/1005003263865347.html">smaller</a> ones will be used for workshops by arcbtc).
* <a href="https://www.amazon.co.uk/Sourcingmap-2x40-Pins-Connector-Arduino-Prototype-Black-Silver-Tone/dp/B07DK532DP/ref=sr_1_1_sspa?dchild=1&keywords=angled+gpio+pins&qid=1633810409&sr=8-1-spons&psc=1&smid=AIF4G7PLKBOZY&spLa=ZW5jcnlwdGVkUXVhbGlmaWVyPUEzQTVQOFdIWUVaMFBBJmVuY3J5cHRlZElkPUEwMTgzMjU2Mk40RUlST0pQRlRVOCZlbmNyeXB0ZWRBZElkPUEwMzQ3MTM3WlBPNEgxSjFKMEFRJndpZGdldE5hbWU9c3BfYXRmJmFjdGlvbj1jbGlja1JlZGlyZWN0JmRvTm90TG9nQ2xpY2s9dHJ1ZQ==">Angled male/male GPIO pins</a>

## Arduino software install

* Download/install latest <a href="https://www.arduino.cc/en/software">Arduino IDE</a>
* Install ESP32 boards, using <a href="https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager">boards manager</a>
* Copy <a href="https://github.com/arcbtc/LNURLPoS/tree/main/LNURLPoS/libraries">these libraries</a> into your Arduino IDE library folder
* Plug in T-Display, from *Tools>Board>ESP32 Boards* select **TTGO LoRa32 OLED V1**
> *Note: You may need to roll your ESP32 boards back to an earlier version in the Arduino IDE, by using tools>boards>boards manager, searching for esp. I use v1.0.5(rc6), and have also used v1.0.4 which worked.*
## LNbits extension

To make things easy (usually a few clicks on things like Raspiblitz), there is an <a href="https://github.com/lnbits/lnbits/tree/master/lnbits/extensions/lnurlpos">LNbits extension</a>.
If you want to make your own stand-alone server software that would be fairly easy to do, by replicating the lnurl.py file in the extennsion.

## Case

Recycle the T-Display case, as with the smaller keypad it all fits together perfectly. See <a href="https://twitter.com/arcbtc/status/1442511015669809152">demo</a>

Alternatively, there are 2 lightburn designs for cases depending on membrane keypads. Designs use layered 3mm acrylic and M4 nuts/bolts <br/>(Blue = fill/engrave 1.5mm depth, Black = cut)
<p align="center">
<img src="https://user-images.githubusercontent.com/33088785/134685048-bba3c43f-a454-4459-a6e9-211a60c70ff1.gif" style="width:48%; height: 50%">
</p>
<br/><br/>
Laser cutters are cheap now and should be part of every makers arsenal, these examples were cut on £200 NEJE Master2s 20W, alternatively there are plenty of laser engraving/cutting companies.
<br/><br/>



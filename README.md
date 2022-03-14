# LNURLPoS

## Cheap, offline(!), DIY bitcoin lightning-network PoS

### for a more up to date version of lnurlpos, checkout <a href="https://github.com/arcbtc/bitcoinPoS">bitcoinPoS</a>

![lnurlpos](https://user-images.githubusercontent.com/33088785/134943216-1a9f3ab6-09da-4e15-b16a-2c2f8bc918da.png)

Lightning-network uses hot wallets and real-world payments are made from phones. The burden of connectivity can be taken away from the point-of-sale and given to the phone.

![image](https://user-images.githubusercontent.com/33088785/134652952-cf5c95ac-afc2-4175-8d09-a983c3f066bc.png)

> <i>Join our <a href="https://t.me/makerbits">telegram support/chat</a>.</i>

> <i>For a traditional PoS experience see my <a href="https://github.com/arcbtc/LNPoS">LNPoS project</a>.</i>

> <i>The manufacturer of the microcontroller used have actually released a specific kit for LNURLPoS! <a href="https://github.com/Xinyuan-LilyGO/T-Display-keyboard">LiLyGos repo</a>.</i>

LNURLPoS uses the LNURL-pay protocol. LNURL-pay allows your lightning-wallet to make a secure request to a server to get a lightning-network invoice. So instead of scanning a massive ugly lightning-network invoice QR, you can scan a lovely little LNURL QR (if you <a href="https://lnurl.fiatjaf.com/codec/">decode</a> an LNURL you'll see its just a URL).

![image](https://user-images.githubusercontent.com/33088785/134657379-bd9e18f0-0289-498b-a1fc-c084c60e64e3.png)

For online stuff I suppose massive QR codes are not an issue, but when fiddling with hardware devices they are. LNURLPoS using the LNURL-pay protocol, it can use a smaller screen for displaying the QR.

### Setup workflow

- LNURLPoS server set up and register PoS in a few clicks on <a href="https://github.com/lnbits/lnbits/">LNbits</a> using the <a href="https://github.com/lnbits/lnbits/tree/master/lnbits/extensions/lnurlpos">LNURLPoS</a> extension
- Copy credentials (including a secret key) from server to the physical LNURLPoS device

### Payment workflow

- Merchant enters amount into LNURLPoS device
- LNURL is generated in device and displayed for scanning (LNURL includes a unique pin encrypted using the secret key shared with the server)
- Customer scans and pays
- When the payment has cleared the customer is sent the decrypted unique pin
- Merchant can compare and verify using the same pin displayed on the lNURLPoS

### Credit/props

Stepan Snigerev for creating beautiful crypto and LNURL encoding functions.

Fiatjafs incredible <a href="https://github.com/lnbits/lnbits/tree/master/lnbits/extensions/offlineshop">OfflineShop</a> extension. LNURLPoS is the same concept, but can run at scale, and is dependent on a device.

<a href="https://www.bleskomat.com/">Belskomat</a> for pironeering the idea of a shared secret for the microcontroller to encrypt data with.

# ⚡⚡⚡⚡⚡⚡ LNURLPoS Tutorial ⚡⚡⚡⚡⚡⚡

<a href="https://www.youtube.com/watch?v=ofCv2cHZ5b0&list=PLPj3KCksGbSYcLQoQbRGAtuHnQ4U4mXeL&index=38" target="_blank" ><img src="https://user-images.githubusercontent.com/33088785/136672544-658d7fdc-2ed4-4442-9536-eaa3eeaf613a.png"></a>

### Hardware needed

- <a href="https://www.aliexpress.com/item/33048962331.html">Lilygo TTGO T-Display</a>
- Keypad membrane (these <a href="https://www.aliexpress.com/item/32812109541.html">big</a> ones are easy to find, these <a href="https://www.aliexpress.com/item/1005003263865347.html">smaller</a> ones will be used for workshops by arcbtc).
- <a href="https://www.amazon.co.uk/Sourcingmap-2x40-Pins-Connector-Arduino-Prototype-Black-Silver-Tone/dp/B07DK532DP/ref=sr_1_1_sspa?dchild=1&keywords=angled+gpio+pins&qid=1633810409&sr=8-1-spons&psc=1&smid=AIF4G7PLKBOZY&spLa=ZW5jcnlwdGVkUXVhbGlmaWVyPUEzQTVQOFdIWUVaMFBBJmVuY3J5cHRlZElkPUEwMTgzMjU2Mk40RUlST0pQRlRVOCZlbmNyeXB0ZWRBZElkPUEwMzQ3MTM3WlBPNEgxSjFKMEFRJndpZGdldE5hbWU9c3BfYXRmJmFjdGlvbj1jbGlja1JlZGlyZWN0JmRvTm90TG9nQ2xpY2s9dHJ1ZQ==">Angled male/male GPIO pins</a>

### Arduino software install

- Download/install latest <a href="https://www.arduino.cc/en/software">Arduino IDE</a>
- Install ESP32 boards, using <a href="https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager">boards manager</a>
- Copy <a href="https://github.com/arcbtc/LNURLPoS/tree/main/LNURLPoS/libraries">these libraries</a> into your Arduino IDE library folder
- Plug in T-Display, from _Tools>Board>ESP32 Boards_ select **TTGO LoRa32 OLED V1**

> _Note: If using MacOS, you will need the CP210x USB to UART Bridge VCP Drivers available here https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers_

### LNbits extension

To make things easy (usually a few clicks on things like Raspiblitz), there is an <a href="https://github.com/lnbits/lnbits/tree/master/lnbits/extensions/lnurlpos">LNbits extension</a>.
If you want to make your own stand-alone server software that would be fairly easy to do, by replicating the lnurl.py file in the extennsion.

### Future updates

At the beginning of this article I said "LNURLPoS (currently) only uses LNURL-Pay". The next stage will be for the PoS to also create LNURL-Withdraws, which are essentially faucets. This means merchants can offer refunds, and also sell bitcoin over the counter, which creates an extremely powerful tool for local economies on-ramping and off-ramping from their local fiat currency.

At Adopting Bitcoin in San Salvador I will distribute 40 kits over x2 workshops, so hopefully some locals will start producing, selling and teaching others how to make these useful little units.

---

## Deeper Dive

### LNURL

Much of the innovation that happens on lightning-network uses an additional protocol layer called LNURL.

LNURL is just a bech32 encoded URL string, that is a link to an LNURL server that your lightning wallet can request information from. By your wallet being able to communicate with a server, developers are no longer bound by the _payee-generate-invoice_ workflow. There are many different types of LNURL. LNURLPoS (currently) only uses LNURL-Pay.

This is an LNURL-pay QR code:

![image](https://user-images.githubusercontent.com/33088785/136535559-7e351bac-781d-4a53-9643-fa8ebcad7aef.png)

This is the data in that QR code:

`LNURL1DP68GURN8GHJ7MRWVF5HGUEWVDHK6TMVDE6HYMRS9ASHQ6F0WCCJ7MRWW4EXCTECXVUQR8PUDZ`

If we decode the LNURL we get this URL:

`https://lnbits.com/lnurlp/api/v1/lnurl/838`

If you do a GET request to the URL this data will be returned (you can test this by just visiting the URL in a browser):

```
{
  "callback": "https://lnbits.com/lnurlp/api/v1/lnurl/cb/838",
  "maxSendable": 10000000000,
  "metadata": "[[\"text/plain\", \"Lovely little QR\"]]",
  "minSendable": 10000,
  "tag": "payRequest"
}
```

When your wallet gets this json it asks you how much you want to send between the `minSendable` and `maxSendable`.

After a moment you get a “payment sent” confirmation and receipt.

<img style="width:40%" src="https://user-images.githubusercontent.com/33088785/136538546-cdc64c22-f7b5-44e4-841e-3341f6865687.png">

So what happened?

When you verify you want to send say 10sats, your wallet sends that data (as a json) to the `callback` URL. The server then generates an invoice for that amount and sends it back to your wallet, which pays it. Once the payment has cleared, the wallet reveals a receipt to you.

### LNURLPoS workflow

LNURLPoS generates and encodes the LNURL in the device, which means we can pass some data in the URL.

The LNURLPoS stores four important pieces of data:

- URL to your LNURL server (we’re using an LNbits install, with dedicated extension)
- PoS ID (Unique ID generated in the LNbits extension)
- Secret (Secret shared with the LNURL server)
- Currency denomination (being offline sats becomes too volatile)

LNURLPoS could use any LNURL server that performs some certain functions, but to make things easy I made an extension in LNbits specifically for LNURLPoS

![image](https://user-images.githubusercontent.com/33088785/136534237-d1ec7a3f-7937-43b0-91d7-b5cd9587389b.png)
![image](https://user-images.githubusercontent.com/33088785/136534873-92d080d5-7825-4358-8dd5-3d04009aa80b.png)

Once a PoS has been generated the extension gives you this data:

```
String server = "https://lnbits.com";
String posId = "L4aJNiQZyPxCREoB3KXiiU";
String key = "4TPLxRmv82yEFjUgWKdfPh";
String currency = "EUR";
```

The data can then be passed to the device when uploading its software through the Arduino IDE

<img style="width:50%" src="https://user-images.githubusercontent.com/33088785/136539934-32b47c21-67c4-44bc-976b-c8ccf399570e.png">

When an amount is entered into the LNURLPoS, the device generates a unique pin, then encrypts the amount+pin using the shared secret and a nonce. The server endpoint, nonce, encypted data, and PoS ID are built into into the LNURL.

`https://lnbits.com/lnurlpos/api/v1/lnurl/<nonce>/<encrypted-data>/<pos-id>`

`LNURL1DP68GURN8GHJ7MRWVF5HGUEWVDHK6TMVDE6HYMRSDAEJ7CTSDYHHVVF0D3H82UNV9U7XUMMWVDJNUTEUV4HXXUNEWP6X2EPDV3SHGCF79U78QMMN945KG0S2PG6GTWSK`

When that first GET request happens from the wallet, the LNURL server can find the PoS record, fetch its secret use the secret to decrypt the amount+pin. The amount is converted from the fiat currency to sats, and sent back to the wallet as `minSendable` and `maxSendable`.

If the invoice passed to the wallet is paid the customer is given access to the decrypted pin.

<img style="width:40%" src="https://user-images.githubusercontent.com/33088785/136544780-10f19ab3-ee47-4b46-aa40-7d983dbf14a8.png">

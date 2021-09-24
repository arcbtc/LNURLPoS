# LNURLPoS
## Offline lightning PoS

Lightning-network uses a hot wallets, so all real-world payments are made from phones. The burden of connectivity can be taken away from the point-of-sale and given to the phone.


![image](https://user-images.githubusercontent.com/33088785/134652952-cf5c95ac-afc2-4175-8d09-a983c3f066bc.png)

<i>For a traditional PoS experience see my <a href="https://github.com/arcbtc/LNPoS">LNPoS project</a>.</i>

LNURLPoS uses the LNURL-pay protocol. LNURL-pay allows your lightning-wallet to make a secure request to a server to get a lightning-network invoice. So instead of scanning a massive ugly lightning-network invoice QR, you can scan a lovely little LNURL QR (if you <a href="https://lnurl.fiatjaf.com/codec/">decode</a> an LNURL you'll see its just a URL).  

![image](https://user-images.githubusercontent.com/33088785/134657379-bd9e18f0-0289-498b-a1fc-c084c60e64e3.png)

For online stuff I suppose massive QR codes are not an issue, but for me fiddling with devices they are. By LNURLPoS using the lNURL-pay protocol, it can also use a smaller screen for displaying the QR.


# Micro-Bitcoin

C++ Bitcoin library for 32-bit microcontrollers. The library supports [Arduino IDE](https://www.arduino.cc/), [ARM mbed](https://www.mbed.com/en/) and bare metal.<br>
It provides a collection of convenient classes for Bitcoin: private and public keys, HD wallets, generation of the recovery phrases, PSBT transaction formats, scripts — everything required for a hardware wallet or other bitcoin-powered device.

The library should work on any decent 32-bit microcontroller, like esp32, riscV, stm32 series and others. It *doesn't work* on 8-bit microcontrollers like a classic Arduino as these microcontrollers are not powerful enough to run complicated crypto algorithms.

We use elliptic curve implementation from [trezor-crypto](https://github.com/trezor/trezor-firmware/tree/master/crypto). API is inspired by [Jimmy Song's](https://github.com/jimmysong/) Porgramming Blockchain class and the [book](https://github.com/jimmysong/programmingbitcoin).

## Documentation

Check out our [tutorial](https://micro-bitcoin.github.io/#/tutorial/README) where we write a minimal hardware wallet, or browse the [API docs](https://micro-bitcoin.github.io/#/api/README). We also have a collection of [recepies](https://micro-bitcoin.github.io/#/recepies/README) for some common use-cases.

Telegram group: https://t.me/arduinoBitcoin

## Alternative libraries

[DIY Bitcoin Hardware website](https://diybitcoinhardware.com/) has a nice collection of bitcoin-related projects, resources and libraries for makers.

A few bitcoin libraries:

- [secp256k1](https://github.com/bitcoin-core/secp256k1) — elliptic curve library  from Bitcoin Core and a [version](https://github.com/diybitcoinhardware/secp256k1-embedded) working with Arduino IDE & Mbed out of the box.
- [libwally](https://github.com/ElementsProject/libwally-core/) - bitcoin library from Blockstream and a [version](https://github.com/diybitcoinhardware/libwally-embedded) working with Arduino IDE.
- [f469-disco](https://github.com/diybitcoinhardware/f469-disco) - micropython bitcoin bundle for STM Discovery board and other platforms.

## Installation

The library is not listed in the Arduino Library manager yet, but you can download and install it manually.

[Download](https://github.com/micro-bitcoin/uBitcoin/archive/master.zip) the zip file from our [repository](https://github.com/micro-bitcoin/uBitcoin/) and select in Arduino IDE `Sketch` → `Include library` → `Add .ZIP library...`.

Or clone it into your `Documents/Arduino/libraries` folder:

```sh
git clone https://github.com/micro-bitcoin/uBitcoin.git
```

When installed you will also see a few examples in `File` → `Examples` → `Bitcoin` menu.

## Basic usage example

First, don't forget to include necessary headers:

```cpp
// we use these two in our sketch:
#include "Bitcoin.h"
#include "PSBT.h"       // if using PSBT functionality
// other headers of the library
#include "Conversion.h" // to get access to functions like toHex() or fromBase64()
#include "Hash.h"       // if using hashes in your code
```

Now we can write a simple example that does the following:

1. Creates a master private key from a recovery phrase and empty password
2. Derives account and prints master public key for a watch-only wallet (`zpub` in this case)
3. Derives and print first segwit address
4. Parses, signs and prints signed PSBT transaction

```cpp
// derive master private key
HDPrivateKey hd("add good charge eagle walk culture book inherit fan nature seek repair", "");
// derive native segwit account (bip-84) for tesnet
HDPrivateKey account = hd.derive("m/84'/1'/0'/");
// print xpub: vpub5YkPqVJTA7gjK...AH2rXvcAe3p781G
Serial.println(account.xpub());
// or change the account type to UNKNOWN_TYPE to get tpub
HDPublicKey xpub = account.xpub();
xpub.type = UNKNOWN_TYPE;
// this time prints tpubDCnYy4Ty...dL4fLKsBFjFQwz
Serial.println(xpub);
// set back correct type to get segwit addresses by default
xpub.type = P2WPKH;
Serial.println(hd.fingerprint());

// print first address: tb1q6c8m3whsag5zadgl32nmhuf9q0qmtklws25n6g
Serial.println(xpub.derive("m/0/0").address());

PSBT tx;
// parse unsigned transaction
tx.parseBase64("cHNidP8BAHECAAAAAUQS8FqBzYocPDpeQmXBRBH7NwZHVJF39dYJDCXxq"
"zf6AAAAAAD+////AqCGAQAAAAAAFgAUuP0WcSBmiAZYi91nX90hg/cZJ1U8AgMAAAAAABYAF"
"C1RhUR+m/nFyQkPSlP0xmZVxlOqAAAAAAABAR/gkwQAAAAAABYAFNYPuLrw6igutR+Kp7vxJ"
"QPBtdvuIgYDzkBZaAkSIz0P0BexiPYfzInxu9mMeuaOQa1fGEUXcWIYoyAeuFQAAIABAACAA"
"AAAgAAAAAAAAAAAAAAiAgMxjOiFQofq7l9q42nsLA3Ta4zKpEs5eCnAvMnQaVeqsBijIB64V"
"AAAgAEAAIAAAACAAQAAAAAAAAAA");
// sign with the root key
tx.sign(hd);
// print signed transaction
Serial.println(tx.toBase64());
```

Ready for more? Check out the [tutorial](https://micro-bitcoin.github.io/#/tutorial/README) and start writing your very own hardware wallet!
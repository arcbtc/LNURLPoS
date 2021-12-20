#ifndef __ELECTRUM_H__
#define __ELECTRUM_H__

#include "Bitcoin.h"

/**
 * \brief Metadata for Electrum unsigned transaction format to determine how to sign transaction.
 *        Only partially supported - 2 derivation indexes and HD public key. Segwit and legacy.
 */
typedef struct{
    HDPublicKey hd;
    uint16_t derivation[2];
    uint64_t amount;
} ElectrumInputMetadata;
/**
 *  \brief Electrum Partially Signed Transaction class.<br>
 *         WARNING: Only partial support is implemented! No multisig and single keys.<br>
 *         For now implements only single HD key transactions are supported (no multisig).<br>
 *         If you explain me how electrum encodes unsigned multisig I will make it.
 */
class ElectrumTx : public Streamable{
protected:
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const{ return s->serialize(&tx, offset); };
    bool is_segwit;
public:
    ElectrumTx(){ txInsMeta = NULL; is_segwit = false; };
    ElectrumTx(ElectrumTx const &other);
    ~ElectrumTx();
    Tx tx;
    virtual size_t length() const{ return tx.length(); };

    /** \bried metadata for inputs */
    ElectrumInputMetadata * txInsMeta;

    /** \brief signs all inputs with matching hd pubkey with account HDPrivateKey.
     *         Returns number of inputs signed.
     */
    uint8_t sign(const HDPrivateKey account);
    /** \brief calculates fee if input amounts are known */
    uint64_t fee() const;

    ElectrumTx &operator=(ElectrumTx const &other);
    
    bool isValid() const{ return status==PARSING_DONE; };
    explicit operator bool() const{ return isValid(); };
};

#endif // __ELECTRUM_H__

package org.bitcoin;

import com.google.common.io.BaseEncoding;
import java.util.Arrays;
import java.math.BigInteger;
import javax.xml.bind.DatatypeConverter;
import static org.bitcoin.NativeSecp256k1Util.*;

/**
 * This class holds test cases defined for testing this library.
 */
public class NativeSecp256k1Test {

    //TODO improve comments/add more tests
    /**
      * This tests verify() for a valid signature
      */
    public static void testVerifyPos() throws AssertFailException{
        boolean result = false;
        byte[] data = BaseEncoding.base16().lowerCase().decode("CF80CD8AED482D5D1527D7DC72FCEFF84E6326592848447D2DC0B0E87DFC9A90".toLowerCase()); //sha256hash of "testing"
        byte[] sig = BaseEncoding.base16().lowerCase().decode("3044022079BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F817980220294F14E883B3F525B5367756C2A11EF6CF84B730B36C17CB0C56F0AAB2C98589".toLowerCase());
        byte[] pub = BaseEncoding.base16().lowerCase().decode("040A629506E1B65CD9D2E0BA9C75DF9C4FED0DB16DC9625ED14397F0AFC836FAE595DC53F8B0EFE61E703075BD9B143BAC75EC0E19F82A2208CAEB32BE53414C40".toLowerCase());

        result = NativeSecp256k1.verify( data, sig, pub);
        assertEquals( result, true , "testVerifyPos");
    }

    /**
      * This tests verify() for a non-valid signature
      */
    public static void testVerifyNeg() throws AssertFailException{
        boolean result = false;
        byte[] data = BaseEncoding.base16().lowerCase().decode("CF80CD8AED482D5D1527D7DC72FCEFF84E6326592848447D2DC0B0E87DFC9A91".toLowerCase()); //sha256hash of "testing"
        byte[] sig = BaseEncoding.base16().lowerCase().decode("3044022079BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F817980220294F14E883B3F525B5367756C2A11EF6CF84B730B36C17CB0C56F0AAB2C98589".toLowerCase());
        byte[] pub = BaseEncoding.base16().lowerCase().decode("040A629506E1B65CD9D2E0BA9C75DF9C4FED0DB16DC9625ED14397F0AFC836FAE595DC53F8B0EFE61E703075BD9B143BAC75EC0E19F82A2208CAEB32BE53414C40".toLowerCase());

        result = NativeSecp256k1.verify( data, sig, pub);
        //System.out.println(" TEST " + new BigInteger(1, resultbytes).toString(16));
        assertEquals( result, false , "testVerifyNeg");
    }

    /**
      * This tests secret key verify() for a valid secretkey
      */
    public static void testSecKeyVerifyPos() throws AssertFailException{
        boolean result = false;
        byte[] sec = BaseEncoding.base16().lowerCase().decode("67E56582298859DDAE725F972992A07C6C4FB9F62A8FFF58CE3CA926A1063530".toLowerCase());

        result = NativeSecp256k1.secKeyVerify( sec );
        //System.out.println(" TEST " + new BigInteger(1, resultbytes).toString(16));
        assertEquals( result, true , "testSecKeyVerifyPos");
    }

    /**
      * This tests secret key verify() for an invalid secretkey
      */
    public static void testSecKeyVerifyNeg() throws AssertFailException{
        boolean result = false;
        byte[] sec = BaseEncoding.base16().lowerCase().decode("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF".toLowerCase());

        result = NativeSecp256k1.secKeyVerify( sec );
        //System.out.println(" TEST " + new BigInteger(1, resultbytes).toString(16));
        assertEquals( result, false , "testSecKeyVerifyNeg");
    }

    /**
      * This tests public key create() for a valid secretkey
      */
    public static void testPubKeyCreatePos() throws AssertFailException{
        byte[] sec = BaseEncoding.base16().lowerCase().decode("67E56582298859DDAE725F972992A07C6C4FB9F62A8FFF58CE3CA926A1063530".toLowerCase());

        byte[] resultArr = NativeSecp256k1.computePubkey( sec);
        String pubkeyString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
        assertEquals( pubkeyString , "04C591A8FF19AC9C4E4E5793673B83123437E975285E7B442F4EE2654DFFCA5E2D2103ED494718C697AC9AEBCFD19612E224DB46661011863ED2FC54E71861E2A6" , "testPubKeyCreatePos");
    }

    /**
      * This tests public key create() for a invalid secretkey
      */
    public static void testPubKeyCreateNeg() throws AssertFailException{
       byte[] sec = BaseEncoding.base16().lowerCase().decode("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF".toLowerCase());

       byte[] resultArr = NativeSecp256k1.computePubkey( sec);
       String pubkeyString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
       assertEquals( pubkeyString, "" , "testPubKeyCreateNeg");
    }

    /**
      * This tests sign() for a valid secretkey
      */
    public static void testSignPos() throws AssertFailException{

        byte[] data = BaseEncoding.base16().lowerCase().decode("CF80CD8AED482D5D1527D7DC72FCEFF84E6326592848447D2DC0B0E87DFC9A90".toLowerCase()); //sha256hash of "testing"
        byte[] sec = BaseEncoding.base16().lowerCase().decode("67E56582298859DDAE725F972992A07C6C4FB9F62A8FFF58CE3CA926A1063530".toLowerCase());

        byte[] resultArr = NativeSecp256k1.sign(data, sec);
        String sigString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
        assertEquals( sigString, "30440220182A108E1448DC8F1FB467D06A0F3BB8EA0533584CB954EF8DA112F1D60E39A202201C66F36DA211C087F3AF88B50EDF4F9BDAA6CF5FD6817E74DCA34DB12390C6E9" , "testSignPos");
    }

    /**
      * This tests sign() for a invalid secretkey
      */
    public static void testSignNeg() throws AssertFailException{
        byte[] data = BaseEncoding.base16().lowerCase().decode("CF80CD8AED482D5D1527D7DC72FCEFF84E6326592848447D2DC0B0E87DFC9A90".toLowerCase()); //sha256hash of "testing"
        byte[] sec = BaseEncoding.base16().lowerCase().decode("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF".toLowerCase());

        byte[] resultArr = NativeSecp256k1.sign(data, sec);
        String sigString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
        assertEquals( sigString, "" , "testSignNeg");
    }

    /**
      * This tests private key tweak-add
      */
    public static void testPrivKeyTweakAdd_1() throws AssertFailException {
        byte[] sec = BaseEncoding.base16().lowerCase().decode("67E56582298859DDAE725F972992A07C6C4FB9F62A8FFF58CE3CA926A1063530".toLowerCase());
        byte[] data = BaseEncoding.base16().lowerCase().decode("3982F19BEF1615BCCFBB05E321C10E1D4CBA3DF0E841C2E41EEB6016347653C3".toLowerCase()); //sha256hash of "tweak"

        byte[] resultArr = NativeSecp256k1.privKeyTweakAdd( sec , data );
        String sigString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
        assertEquals( sigString , "A168571E189E6F9A7E2D657A4B53AE99B909F7E712D1C23CED28093CD57C88F3" , "testPrivKeyAdd_1");
    }

    /**
      * This tests private key tweak-mul
      */
    public static void testPrivKeyTweakMul_1() throws AssertFailException {
        byte[] sec = BaseEncoding.base16().lowerCase().decode("67E56582298859DDAE725F972992A07C6C4FB9F62A8FFF58CE3CA926A1063530".toLowerCase());
        byte[] data = BaseEncoding.base16().lowerCase().decode("3982F19BEF1615BCCFBB05E321C10E1D4CBA3DF0E841C2E41EEB6016347653C3".toLowerCase()); //sha256hash of "tweak"

        byte[] resultArr = NativeSecp256k1.privKeyTweakMul( sec , data );
        String sigString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
        assertEquals( sigString , "97F8184235F101550F3C71C927507651BD3F1CDB4A5A33B8986ACF0DEE20FFFC" , "testPrivKeyMul_1");
    }

    /**
      * This tests private key tweak-add uncompressed
      */
    public static void testPrivKeyTweakAdd_2() throws AssertFailException {
        byte[] pub = BaseEncoding.base16().lowerCase().decode("040A629506E1B65CD9D2E0BA9C75DF9C4FED0DB16DC9625ED14397F0AFC836FAE595DC53F8B0EFE61E703075BD9B143BAC75EC0E19F82A2208CAEB32BE53414C40".toLowerCase());
        byte[] data = BaseEncoding.base16().lowerCase().decode("3982F19BEF1615BCCFBB05E321C10E1D4CBA3DF0E841C2E41EEB6016347653C3".toLowerCase()); //sha256hash of "tweak"

        byte[] resultArr = NativeSecp256k1.pubKeyTweakAdd( pub , data );
        String sigString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
        assertEquals( sigString , "0411C6790F4B663CCE607BAAE08C43557EDC1A4D11D88DFCB3D841D0C6A941AF525A268E2A863C148555C48FB5FBA368E88718A46E205FABC3DBA2CCFFAB0796EF" , "testPrivKeyAdd_2");
    }

    /**
      * This tests private key tweak-mul uncompressed
      */
    public static void testPrivKeyTweakMul_2() throws AssertFailException {
        byte[] pub = BaseEncoding.base16().lowerCase().decode("040A629506E1B65CD9D2E0BA9C75DF9C4FED0DB16DC9625ED14397F0AFC836FAE595DC53F8B0EFE61E703075BD9B143BAC75EC0E19F82A2208CAEB32BE53414C40".toLowerCase());
        byte[] data = BaseEncoding.base16().lowerCase().decode("3982F19BEF1615BCCFBB05E321C10E1D4CBA3DF0E841C2E41EEB6016347653C3".toLowerCase()); //sha256hash of "tweak"

        byte[] resultArr = NativeSecp256k1.pubKeyTweakMul( pub , data );
        String sigString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
        assertEquals( sigString , "04E0FE6FE55EBCA626B98A807F6CAF654139E14E5E3698F01A9A658E21DC1D2791EC060D4F412A794D5370F672BC94B722640B5F76914151CFCA6E712CA48CC589" , "testPrivKeyMul_2");
    }

    /**
      * This tests seed randomization
      */
    public static void testRandomize() throws AssertFailException {
        byte[] seed = BaseEncoding.base16().lowerCase().decode("A441B15FE9A3CF56661190A0B93B9DEC7D04127288CC87250967CF3B52894D11".toLowerCase()); //sha256hash of "random"
        boolean result = NativeSecp256k1.randomize(seed);
        assertEquals( result, true, "testRandomize");
    }

    public static void testCreateECDHSecret() throws AssertFailException{

    //     byte[] sec = BaseEncoding.base16().lowerCase().decode("67E56582298859DDAE725F972992A07C6C4FB9F62A8FFF58CE3CA926A1063530".toLowerCase());
    //     byte[] pub = BaseEncoding.base16().lowerCase().decode("040A629506E1B65CD9D2E0BA9C75DF9C4FED0DB16DC9625ED14397F0AFC836FAE595DC53F8B0EFE61E703075BD9B143BAC75EC0E19F82A2208CAEB32BE53414C40".toLowerCase());

    //     byte[] resultArr = NativeSecp256k1.createECDHSecret(sec, pub);
    //     String ecdhString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
    //     assertEquals( ecdhString, "2A2A67007A926E6594AF3EB564FC74005B37A9C8AEF2033C4552051B5C87F043" , "testCreateECDHSecret");
    }

    public static void testAdaptorSign() throws AssertFailException {
        byte[] msg = BaseEncoding.base16().lowerCase().decode("44b9c3dd8ea1dacbf5903d99e04f1a3edf9f5ce919677558aac0b88cfeba61b6");
        byte[] adaptor = BaseEncoding.base16().lowerCase().decode("1649fa88e6311558dc1e537d4095ad802da9371e0c38e6fa16a0d52891b037abd4fa3cf3adae3985cb2e3e87dc1f4adacca4cbed39bfe8a81ff8b2fb5a0dca48");
        byte[] seckey = BaseEncoding.base16().lowerCase().decode("5532eaf258e01a17c9c89df5597436b1e57f13193fc31eef74cd159920b0364a");
        String expectedAdaptorSig = "0011bcdfd0b0f79f1a7fccca6b47341e77abb77333e203be0da7d95ee67ecce204715f55bb4606aae05aaf92c6eeea41fcdef886f623fdfd7d246a364582380430";
        String expectedAdaptorProof = "018e34839a35355cdc14290e4732756a473202d8d9d65196c5947a73fb20195249d32fd8e893e2dd3a748ae910595d25170f736cff3cb905fe1c2d40f694666b83d2a61f8a62dd97a62029513630949aff3510c6f48689738aa0b13180c44f8df2";

        byte[] resultArr = NativeSecp256k1.adaptorSign(msg, adaptor, seckey);

        assertEquals(resultArr.length, 2, "testAdaptorSign");

        String adaptorSig = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr).toLowerCase();
        assertEquals(adaptorSig, expectedAdaptorSig + expectedAdaptorProof, "testAdaptorSign");
    }

    public static void testAdaptorVeirfy() throws AssertFailException {
        byte[] msg = BaseEncoding.base16().lowerCase().decode("e4fd4ad9bde7bbaf3850b8263a74429f506720e4a599b0550fb92e4126ae1480");
        byte[] adaptorSig = BaseEncoding.base16().lowerCase().decode("00e41360875c3d1bf3629a7838911726998a0e7afce8734acb1569f6f1bcd4462fbd7d1d079e613a62dd7af98b548f154d46b9829443c7ec1e5e13922af4bf3c0c");
        byte[] adaptorProof = BaseEncoding.base16().lowerCase().decode("0107d1ab8c8be7c5be077b66bf34b9adc87d927add5575c38cfbf8fa5106d36848b46e4fb0834ff9a8c4da21a243be1372f1da9a91ce8079739bd535445d85582f778bba7d320408ab8e22c9892c9950acb2fd04eacff64898f45dc80ee05bc7c8");
        byte[] adaptor = BaseEncoding.base16().lowerCase().decode("a4580aac6111cd16d7fb57cec599d2ddedbfec553a765dc7c5b89e6fe4be5b5b79f6c912d84b557c3641dedd1c16bf8410043a37fbbe9ac2fe3670098081b8e8");
        byte[] pubkey = BaseEncoding.base16().lowerCase().decode("017b97a354756874f76297639f4ce2976481a67205ab983322fa4220a61ad9697e83bde87f5a7228b69b6f70bb631e72c603c51593d25af1ed502df9aefd4e8e");

        boolean result = NativeSecp256k1.adaptorVerify(msg, adaptorSig, pubkey, adaptor, adaptorProof);

        assertEquals( result, true , "testAdaptorVeirfy");
    }

    public static void testAdaptorAdapt() throws AssertFailException {
        byte[] secret = BaseEncoding.base16().lowerCase().decode("5532eaf258e01a17c9c89df5597436b1e57f13193fc31eef74cd159920b0364a");
        byte[] adaptorSig = BaseEncoding.base16().lowerCase().decode("0011bcdfd0b0f79f1a7fccca6b47341e77abb77333e203be0da7d95ee67ecce204715f55bb4606aae05aaf92c6eeea41fcdef886f623fdfd7d246a364582380430");

        byte[] resultArr = NativeSecp256k1.adaptorAdapt(secret, adaptorSig);

        String sigString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
        assertEquals(sigString , "04E2CC7EE65ED9A70DBE03E23373B7AB771E34476BCACC7F1A9FF7B0D0DFBC117FBA6A1964D46E620539FE907C53D2344A336B9270FB0CDC3305C967203DB17F5F7D5923572E4700" , "testAdaptorAdapt");
    }

    public static void testAdaptorExtractSecret() throws AssertFailException {
        byte[] sig = BaseEncoding.base16().lowerCase().decode("04E2CC7EE65ED9A70DBE03E23373B7AB771E34476BCACC7F1A9FF7B0D0DFBC117FBA6A1964D46E620539FE907C53D2344A336B9270FB0CDC3305C967203DB17F".toLowerCase());
        byte[] adaptorSig = BaseEncoding.base16().lowerCase().decode("00e41360875c3d1bf3629a7838911726998a0e7afce8734acb1569f6f1bcd4462fbd7d1d079e613a62dd7af98b548f154d46b9829443c7ec1e5e13922af4bf3c0c");
        byte[] adaptor = BaseEncoding.base16().lowerCase().decode("a4580aac6111cd16d7fb57cec599d2ddedbfec553a765dc7c5b89e6fe4be5b5b79f6c912d84b557c3641dedd1c16bf8410043a37fbbe9ac2fe3670098081b8e8");

        byte[] resultArr = NativeSecp256k1.adaptorExtractSecret(sig, adaptorSig, adaptor);

        String sigString = javax.xml.bind.DatatypeConverter.printHexBinary(resultArr);
        assertEquals(sigString , "5532eaf258e01a17c9c89df5597436b1e57f13193fc31eef74cd159920b0364a" , "testAdaptorExtractSecret");
    }

    public static void main(String[] args) throws AssertFailException{


        System.out.println("\n libsecp256k1 enabled: " + Secp256k1Context.isEnabled() + "\n");

        assertEquals( Secp256k1Context.isEnabled(), true, "isEnabled" );

        //Test verify() success/fail
        testVerifyPos();
        testVerifyNeg();

        //Test secKeyVerify() success/fail
        testSecKeyVerifyPos();
        testSecKeyVerifyNeg();

        //Test computePubkey() success/fail
        testPubKeyCreatePos();
        testPubKeyCreateNeg();

        //Test sign() success/fail
        testSignPos();
        testSignNeg();

        //Test privKeyTweakAdd() 1
        testPrivKeyTweakAdd_1();

        //Test privKeyTweakMul() 2
        testPrivKeyTweakMul_1();

        //Test privKeyTweakAdd() 3
        testPrivKeyTweakAdd_2();

        //Test privKeyTweakMul() 4
        testPrivKeyTweakMul_2();

        //Test randomize()
        testRandomize();

        //Test ECDH
        testCreateECDHSecret();

        // Test ECDSA Adaptor Sigs
//        testAdaptorSign();
//        testAdaptorVeirfy();
//        testAdaptorAdapt();
        testAdaptorExtractSecret();

        NativeSecp256k1.cleanup();

        byte[] sigArr = new byte[]{1,2};
        byte[] proofArr = new byte[]{3,4};
        byte[] resArray = new byte[sigArr.length + proofArr.length];
        System.arraycopy(sigArr, 0, resArray, 0, sigArr.length);
        System.arraycopy(proofArr, 0, resArray, sigArr.length, proofArr.length);

        for (byte c : resArray) {
            System.out.println(c);
        }


        System.out.println(" All tests passed." );

    }
}

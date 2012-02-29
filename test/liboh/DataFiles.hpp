#ifndef __SIRIKATA_STRESS_TEST_BUCKET_HPP__
#define __SIRIKATA_STRESS_TEST_BUCKET_HPP__

#include <sirikata/oh/Storage.hpp>

class DataFiles
{
public:
    OH::Storage::ReadSet dataSet;
    String dataString(int len, String s);

    DataFiles();
    void setData(String s);
};

inline String DataFiles::dataString(int len, String s){
    String DATA = s;
    if (len<=1000){
        for (int i=0; i<len-1; i++){
            DATA=DATA+s;
        }
    }
    else{
        String DATA_ = s;
        for (int i=0; i<1000-1; i++) {
            DATA_ = DATA_ + s;
        }
        DATA=DATA_;
        for (int i=0; i<len/1000-1; i++){
            DATA=DATA+DATA_;
        }
    }
    return DATA;
}

inline void DataFiles::setData(String s){
    dataSet[s+"-10"]=dataString(10,s);
    dataSet[s+"-100"]=dataString(100,s);
    dataSet[s+"-1K"]=dataString(1000,s);
    dataSet[s+"-10K"]=dataString(10000,s);
    dataSet[s+"-100K"]=dataString(100000,s);
//  dataSet[s+"-1M"]=dataString(1000000,s);
}

inline DataFiles::DataFiles(){
    setData("a");
    setData("b");
    setData("c");
    setData("d");
    setData("e");
    setData("f");
    setData("g");
    setData("h");
    setData("i");
    setData("j");
    setData("k");
    setData("l");
    setData("m");
    setData("n");
    setData("o");
    setData("p");
    setData("q");
    setData("r");
    setData("s");
    setData("t");
    setData("u");
    setData("v");
    setData("w");
    setData("x");
    setData("y");
    setData("z");
}

#define STORAGE_STRESSTEST_OH_BUCKETS { \
        OH::Storage::Bucket("7aecb4c8-f146-44b8-a737-f3d8c7d75a01", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("49d3188d-5d1d-4fab-8967-11d81db47b1d", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("40814acc-97b5-4093-a22b-ae568925dbb0", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("a9143a4c-d1f4-4270-9089-682bd2fffbd3", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("1d95c094-da8b-490f-878f-711f2047974d", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("260d0395-a04c-4821-ac93-d6adc763be2d", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("4d787397-41bb-4c48-bcac-2f8293f15431", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("18b0e90a-80dc-4859-a255-13dfbfc39974", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("d6b4ec95-9f1f-41db-93be-95947ed55f73", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("4cb4879f-8967-4f59-9ef1-7a5fa87eb767", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("5971d288-b234-4ec1-a626-6d2b9dd1b67a", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("21206909-16c5-4348-b908-12e6d6481759", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("5d817b62-a9f0-456a-b958-46a5fe762479", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("beadb230-4e93-415a-8855-5ca409244e1c", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("b3413169-2a98-4779-a83e-e0226ba0c0d1", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("c6fa0d07-0a76-42ac-95db-6bf62cfdd3c0", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("1dc24b92-9b18-4eb5-8396-40caebdb5b3e", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("e88d5bdb-0eff-4176-b0a9-8a9e780e190e", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("ce191003-2824-4f76-96ce-c8e12763b9de", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("40017961-c373-49fd-84e6-0dd9a0d0007a", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("1729d11b-b425-4a5d-b6db-068034e7258b", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("3e29a157-7fc7-42da-a61f-00fa56523360", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("7fbf1a66-0f22-4eab-9080-e90262aa0a62", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("379c7d78-f84b-4603-9d2f-9d640b2028c2", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("075e5d75-f2ad-4cae-9e40-cf732ae422d7", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("7da23c1c-aa32-4074-a84b-7acf617eb052", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("89161a63-556a-457f-bfe7-1abe9a00a96f", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("4ecf4188-75b1-486d-9d7a-0a8844591f47", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("4138fd81-b656-48d9-9bc5-ff1da918b150", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("efba9b03-2f03-4c44-be8b-398ff37950dc", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("4644c699-3cb7-4b5a-b800-ff02a13f1a42", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("55365575-5e90-415e-a660-29b3c2c84c50", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("1d8b5977-5850-4821-923b-da13d36aaf3c", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("5506d724-042e-44de-bca1-5b24d7258f98", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("70caac53-a644-48aa-8c39-c8a4ca2ff246", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("4d220e69-fac6-4dbf-8087-7e6376fdf608", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("083ca8b1-69f7-4a97-83f0-8b64cb57f06e", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("1fa718c8-e297-4f5e-ae8a-8d306dead638", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("f7b1b3a6-9935-4d3a-b1ec-0e62eb72f6ef", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("19faee9b-aac7-47af-9292-04defdfda8cb", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("cb064b0d-1a85-44f4-86a0-d9504c0425a3", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("dd5ac250-9102-4ba9-a992-5171561e51bf", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("ad915dfa-372a-4bb0-bc09-8586a6dad9a0", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("54428f5a-809a-4dcf-b666-dd9beeb99d4b", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("cf696f35-a695-4b6b-b191-edc48046748f", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("1195b9dd-a23f-458c-95cc-f29cef135194", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("8e1ae438-f3ca-4421-9060-58db00613f03", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("68b6f82e-8c58-4169-8a7f-b824d4c4a38a", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("af81b0b9-3111-4106-bdcb-06a0d4b22959", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("9aeb2199-fe8b-4f37-8c0f-0ec7d777455c", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("3994ca18-855d-4199-8831-bad38446865a", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("2392f7ca-a043-4565-8cba-3b811e566335", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("41d8ca60-94f6-48c9-8887-2e5049dcc24a", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("6d67c1fa-0166-4915-9cec-5f98d9ab3ed0", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("e7302870-627e-4710-aaf6-3a45ecde27ba", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("2daf1d96-a87f-45da-bb27-c62bb4e3d515", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("1d108a60-e6ef-4006-b956-f1728e6ccd27", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("4a8e6b7d-8e1b-4f4f-a8fd-6f239a7e7eb9", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("121dfad3-554d-435d-bc05-44f880a250e6", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("edb42260-a34b-4f6e-badf-163eb3e4973c", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("12db3bab-281c-4917-b697-9d348067ed03", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("6e916cd4-3656-4323-9a3e-84bbc8ce96dd", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("4d1bd01f-c1ea-45c6-b678-a6bd22814915", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("04ae0b68-3fa5-401c-aeea-32cc64298d7c", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("59284185-8113-4c12-94b9-96f1804a8050", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("e9e1e73e-0f81-4975-ae11-9e5b49d9c8ea", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("e1c6eb7b-281a-43d1-8272-5f980aed7745", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("81178ef5-ed6e-4e8a-9998-f5ad350ed177", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("c6946cf8-c93c-49d3-b615-03cf4179de2a", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("d5ad067a-5556-4166-a98f-74e270af79e6", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("3b32587f-fd46-47d8-aab6-864aa3f25ba0", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("9840ec35-6b80-48d4-b843-9702d5a6c083", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("8a7fefc0-8a24-41cb-89b2-c3268c0f822b", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("2a87ad59-d489-43e6-bbbd-9febbaad7178", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("c6b7d699-03b2-464f-a024-743ebc64677a", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("77ad6ddb-2632-492e-ba99-ea440f960294", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("75a8fb5d-912b-4950-86b6-f8fb741e1d40", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("dcf45fda-bad5-4fff-a6c2-7da7dfeeeadb", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("b06307dd-81a4-4891-b289-c14c536ab68f", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("2d08c201-fa38-4e4c-bb27-916229875454", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("632ce6cd-a49a-43fa-b1fd-524618e15693", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("2fa2b830-d12f-489e-ace2-3cdfdfe01829", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("7642d36c-2777-4fea-ba2b-7b2bc007bf94", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("db294bd4-df14-4fd6-8b1d-658e449d5d85", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("090b7a8b-64b5-4f44-b2e3-75ecfb79b164", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("1e607288-c63b-40e3-a9cc-d6bfa7ec23c8", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("c2b576b9-bb24-4b4d-8b64-76c140eeff1a", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("d2db8ee2-4e85-46aa-b8be-d551cb702638", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("8a48bbf2-8975-4f2e-85fa-89bab686aa1e", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("b3317772-cc02-46b9-b048-12bb5405bd8b", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("04feffe4-626e-41fe-bf76-d9a15205fc65", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("8a355dff-3692-4c86-ac5c-883ede8900c7", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("c0f15532-f5e7-4648-be18-e741a87e3de7", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("f3945fe0-7e10-428a-bdc4-58d35d9b65bf", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("5ce97825-9a37-4e9a-a8ac-e0c9cda8566d", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("0cd62dc2-65ca-49e6-85ec-4cd983f522ed", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("f6c5ddd0-3123-4e62-9d76-57c58bc4e389", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("7c253a14-a7a5-4e96-b4be-65a8ad280c94", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("b2d8959a-e5c3-4e0a-ba22-0d83fb274a97", OH::Storage::Bucket::HumanReadable()), \
	    OH::Storage::Bucket("3c7363a5-5475-4425-b437-aae8f1c4c17e", OH::Storage::Bucket::HumanReadable()) \
	}

#endif //__SIRIKATA_STRESS_TEST_BASE_HPP__


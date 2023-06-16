#include <unistd.h>

#include "cbl/CouchbaseLite.h"

char app_service_url[] = "wss://YOUR_CAPELLA_APP_SERVICE_URL";
char app_service_user[] = "YOUR_APP_USER_NAME";
char app_service_password[] = "YOUR_APP_USER_PASSWORD";

int main() {
    // Set logging level
    CBLLog_SetConsoleLevel(kCBLLogVerbose);

    // Error handling
    CBLError err;
    memset(&err, 0, sizeof(err));

    // Connect to CBLite DB
    CBLDatabase* database = CBLDatabase_Open(FLSTR("mydb"), NULL, &err);
    if (!database) {
        fprintf(stderr, "Error opening database (%d / %d)\n", err.domain, err.code);
        FLSliceResult msg = CBLError_Message(&err);
        fprintf(stderr, "%.*s\n", (int)msg.size, (const char *)msg.buf);
        FLSliceResult_Release(msg);
        return -1;
    }

    // Set collection
    CBLCollection* collection = CBLDatabase_DefaultCollection(database, &err);

    // Create a document with key "test::01"
    CBLDocument* mutableDoc = CBLDocument_CreateWithID(FLSTR("test::01"));
    FLMutableDict properties = CBLDocument_MutableProperties(mutableDoc);
    FLMutableDict_SetFloat(properties, FLSTR("oldVersion"), 3.0);
    if (!CBLCollection_SaveDocument(collection, mutableDoc, &err)) {
        return -1;
    }

    // Update a document with key "test::01"
    properties = CBLDocument_MutableProperties(mutableDoc);
    FLMutableDict_SetString(properties, FLSTR("name"), FLSTR("apples"));
    CBLCollection_SaveDocument(collection, mutableDoc, &err);

    // Connection to remote endpoint
    CBLEndpoint* targetEndpoint = CBLEndpoint_CreateWithURL(FLStr(app_service_url), &err);
    if (!targetEndpoint) {
        return -1;
    }

    // Configure the authentication
    CBLReplicatorConfiguration replConfig;
    CBLAuthenticator* basicAuth = CBLAuth_CreatePassword(FLStr(app_service_user), FLStr(app_service_password));
    memset(&replConfig, 0, sizeof(replConfig));
    replConfig.database = database;
    replConfig.endpoint = targetEndpoint;
    replConfig.authenticator = basicAuth;

    // Create the replicator
    CBLReplicator* replicator = CBLReplicator_Create(&replConfig, &err);
    CBLAuth_Free(basicAuth);
    CBLEndpoint_Free(targetEndpoint);
    if (!replicator) {
        return-1;
    }
    
    // Start the replicator
    CBLReplicator_Start(replicator, false);
    while (true) {
        CBLReplicatorStatus status = CBLReplicator_Status(replicator);
        if (status.activity == kCBLReplicatorStopped) {
            break;
        }
        sleep(5);
    }

    // Read the  document with the key "test::01" and print it
    FLSliceResult json = CBLDocument_CreateJSON(mutableDoc);
    printf("Document in JSON : %.*s\n", (int)json.size, (const char *)json.buf);

    // Delete the document with the key "test::01" and finally release it
    CBLCollection_DeleteDocument(collection, mutableDoc, &err);
    CBLDocument_Release(mutableDoc);

    return 0;
}

#if 0
sqlite3 *db;
char *zErrMsg = 0;
int rc;

rc = sqlite3_open("test.db", &db);

if( rc ) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
} else {
    fprintf(stderr, "Opened database successfully\n");
}

//sqlite3_exec(handle, "PRAGMA locking_mode = EXCLUSIVE",0,0,0);

// 'db' is the pointer you got from sqlite3_open*
// Any (modifying) SQL commands executed here are not committed until at the you call:
char *sql = "DROP TABLE IF EXISTS Cars; CREATE TABLE Cars(Id INT, Name TEXT, Price INT);";
rc = sqlite3_exec(db, sql, 0, 0, 0);


sqlite3_stmt *stmtInsert = 0;
sqlite3_prepare_v2(db, "INSERT INTO Cars VALUES(?1, ?2, ?3);", -1, &stmtInsert, NULL);

sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
for( u32 rowIndex = 0; rowIndex < 5000000; ++rowIndex )
{
    sqlite3_bind_int(stmtInsert, 1, rowIndex );
    sqlite3_bind_text(stmtInsert, 2, "Audi", -1, 0 );
    sqlite3_bind_int(stmtInsert, 3, rowIndex * 3 - 10 );
    rc = sqlite3_step(stmtInsert); 
    sqlite3_reset(stmtInsert);
}

sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
sqlite3_exec(db, "CREATE INDEX testingIndex ON Cars(Id);", NULL, NULL, NULL);



sqlite3_stmt *stmt = 0;
sqlite3_prepare_v2(db, "UPDATE Cars Set Price = 100 where Id = ?1", -1, &stmt, NULL);


time_t start2 = time(0);
sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
for( u32 updateIndex = 0; updateIndex < 1000000; ++updateIndex )
{
    sqlite3_bind_int(stmt, 1, updateIndex );
    rc = sqlite3_step(stmt);
    sqlite3_reset(stmt);
}
sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);

time_t end2 = time(0);
double time = difftime(end2, start2) * 1000.0;

sqlite3_close(db);
#endif


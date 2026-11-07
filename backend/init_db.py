"""Initialize student_records MySQL database and tables from schema.sql."""

import os
from pathlib import Path
import mysql.connector


def init_db():
    config = {
        "host": os.getenv("MYSQL_HOST", "127.0.0.1"),
        "port": int(os.getenv("MYSQL_PORT", "3306")),
        "user": os.getenv("MYSQL_USER", "root"),
        "password": os.getenv("MYSQL_PASSWORD", ""),
        "charset": "utf8mb4",
    }
    ssl_ca = os.getenv("MYSQL_SSL_CA")
    if ssl_ca:
        config.update(ssl_ca=ssl_ca, ssl_verify_cert=True, ssl_verify_identity=True)

    schema_file = Path(__file__).resolve().parent / "schema.sql"
    with open(schema_file, "r", encoding="utf-8") as f:
        sql_content = f.read()

    print(f"Connecting to MySQL at {config['host']}:{config['port']} as '{config['user']}'...")
    connection = mysql.connector.connect(**config)
    cursor = connection.cursor()

    try:
        # Execute each statement separated by semicolon
        for statement in sql_content.split(";"):
            stmt = statement.strip()
            if stmt:
                cursor.execute(stmt)
        connection.commit()
        print("Database 'student_records' and all tables were created successfully from schema.sql!")
    finally:
        cursor.close()
        connection.close()


if __name__ == "__main__":
    init_db()

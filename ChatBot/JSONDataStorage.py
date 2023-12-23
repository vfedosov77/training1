import sqlite3
import json
import os


class JSONDataStorage:
    def __init__(self, db_file):
        self.db_file = db_file
        self.conn = None
        self._initialize_db()

    def _initialize_db(self):
        # Check if the database file exists, and connect to it
        db_exists = os.path.exists(self.db_file)
        self.conn = sqlite3.connect(self.db_file)

        if not db_exists:
            # Create table if the database did not exist
            cursor = self.conn.cursor()
            cursor.execute('''CREATE TABLE data_store
                              (id TEXT PRIMARY KEY, json_data TEXT)''')
            self.conn.commit()

    def insert_json(self, id, data):
        cursor = self.conn.cursor()
        json_str = json.dumps(data)
        cursor.execute("INSERT OR REPLACE INTO data_store (id, json_data) VALUES (?, ?)", (id, json_str))
        self.conn.commit()

    def get_json(self, id):
        cursor = self.conn.cursor()
        cursor.execute("SELECT json_data FROM data_store WHERE id = ?", (id,))
        row = cursor.fetchone()
        return json.loads(row[0]) if row else None

    def close(self):
        self.conn.close()
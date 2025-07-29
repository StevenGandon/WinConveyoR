from fastapi.testclient import TestClient
from api.main import app

client = TestClient(app)

def test_create_and_read_user():
    r = client.post("/api/v1/users", json={"username": "grace", "full_name": "Grace Hopper"})
    assert r.status_code == 200
    data = r.json()
    user_id = data["id"]

    r = client.get(f"/api/v1/users/{user_id}")
    assert r.status_code == 200
    body = r.json()
    assert body["username"] == "grace"
    assert body["full_name"] == "Grace Hopper"

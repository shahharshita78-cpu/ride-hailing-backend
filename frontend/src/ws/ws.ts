type MessageCallback = (data: any) => void;

class WebSocketClient {
  private ws: WebSocket | null = null;
  private url: string;
  private callbacks: Map<string, Set<MessageCallback>> = new Map();
  private token: string | null = null;
  private reconnectAttempts = 0;
  private maxReconnectAttempts = 5;
  private isIntentionalDisconnect = false;

  constructor(url: string) {
    this.url = url;
  }

  connect(token: string) {
    this.token = token;
    this.isIntentionalDisconnect = false;
    
    if (this.ws) {
      this.ws.close();
    }
    const wsUrl = `${this.url}?token=${token}`;
    this.ws = new WebSocket(wsUrl);

    this.ws.onopen = () => {
      console.log('WS Connected');
      this.reconnectAttempts = 0;
    };

    this.ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        const action = data.action || data.event || 'default';
        const callbacks = this.callbacks.get(action);
        if (callbacks) {
          callbacks.forEach((cb) => cb(data));
        }
      } catch (e) {
        console.log('Received raw message:', event.data);
      }
    };

    this.ws.onclose = () => {
      console.log('WS Disconnected');
      if (!this.isIntentionalDisconnect && this.reconnectAttempts < this.maxReconnectAttempts) {
        const backoffMs = Math.min(1000 * Math.pow(2, this.reconnectAttempts), 10000);
        this.reconnectAttempts++;
        setTimeout(() => {
          if (this.token) this.connect(this.token);
        }, backoffMs);
      }
    };
  }

  disconnect() {
    this.isIntentionalDisconnect = true;
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
  }

  send(data: any) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(data));
    }
  }

  on(action: string, callback: MessageCallback) {
    if (!this.callbacks.has(action)) {
      this.callbacks.set(action, new Set());
    }
    this.callbacks.get(action)!.add(callback);
  }

  off(action: string, callback: MessageCallback) {
    const callbacks = this.callbacks.get(action);
    if (callbacks) {
      callbacks.delete(callback);
    }
  }
}

export const wsClient = new WebSocketClient(
  import.meta.env.VITE_WS_URL || 'ws://localhost:8080/api/ws'
);

import { useEffect } from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { useStore } from './store/store';
import { wsClient } from './ws/ws';
import Login from './screens/Login';
import Register from './screens/Register';
import RiderView from './screens/RiderView';
import DriverView from './screens/DriverView';

function App() {
  const { token, user } = useStore();

  useEffect(() => {
    if (token) {
      wsClient.connect(token);
    } else {
      wsClient.disconnect();
    }
    return () => {
      wsClient.disconnect();
    };
  }, [token]);

  return (
    <BrowserRouter>
      <Routes>
        <Route path="/login" element={!user ? <Login /> : <Navigate to="/" />} />
        <Route path="/register" element={!user ? <Register /> : <Navigate to="/" />} />
        
        <Route 
          path="/" 
          element={
            !user ? (
              <Navigate to="/login" />
            ) : user.role === 'DRIVER' ? (
              <DriverView />
            ) : (
              <RiderView />
            )
          } 
        />
      </Routes>
    </BrowserRouter>
  );
}

export default App;

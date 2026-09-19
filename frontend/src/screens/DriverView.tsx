import React, { useState, useEffect } from 'react';
import CustomMap from '../components/Map';
import BottomSheet from '../components/BottomSheet';
import { useStore } from '../store/store';
import api from '../api/api';
import { wsClient } from '../ws/ws';

const defaultCenter: [number, number] = [37.7749, -122.4194]; // SF

const DriverView: React.FC = () => {
  const { ride, setRide, setUser } = useStore();
  const [isOnline, setIsOnline] = useState(false);
  const [driverLocation, setDriverLocation] = useState(defaultCenter);

  useEffect(() => {
    const fetchActiveRide = async () => {
      try {
        const res = await api.get('/rides/active');
        if (res.data && res.data.ride_id) {
          setRide(res.data);
          setIsOnline(true);
        }
      } catch (e) {
        // Ignore
      }
    };
    fetchActiveRide();
  }, [setRide]);

  useEffect(() => {
    const onRideRequest = (data: any) => {
      if (data.event === 'ride_request') {
        setRide({
          ride_id: data.ride_id,
          status: 'REQUESTED',
          pickup: data.pickup,
          destination: data.destination,
          estimated_fare: data.estimated_fare
        });
      }
    };

    if (isOnline) {
      wsClient.on('ride_request', onRideRequest);
    }
    
    return () => {
      wsClient.off('ride_request', onRideRequest);
    };
  }, [isOnline, setRide]);

  useEffect(() => {
    let watchId: number;
    
    if (isOnline && wsClient && 'geolocation' in navigator) {
      watchId = navigator.geolocation.watchPosition(
        (position) => {
          const newLoc: [number, number] = [position.coords.latitude, position.coords.longitude];
          setDriverLocation(newLoc);
        },
        (error) => {
          console.error("Error getting location:", error);
        },
        { enableHighAccuracy: true, maximumAge: 10000, timeout: 5000 }
      );
    }

    return () => {
      if (watchId !== undefined) {
        navigator.geolocation.clearWatch(watchId);
      }
    };
  }, [isOnline]);

  useEffect(() => {
    if (isOnline && wsClient) {
      wsClient.send({ action: 'location', lat: driverLocation[0], lon: driverLocation[1] });
    }
  }, [driverLocation, isOnline]);

  const handleAction = async (actionPath: string, nextStatus: any) => {
    if (!ride.ride_id) return;
    try {
      await api.post(`/rides/${ride.ride_id}${actionPath}`);
      setRide({ status: nextStatus });
    } catch (e: any) {
      console.error(e);
      alert(e.response?.data || 'Action failed');
    }
  };

  const handleLogout = () => {
    setUser(null, null);
  };

  const renderContent = () => {
    if (!ride.status || ride.status === 'COMPLETED' || ride.status === 'CANCELLED') {
      return (
        <div className="space-y-4 text-center py-4">
          <h2 className="text-2xl font-bold">{isOnline ? 'You are Online' : 'You are Offline'}</h2>
          <p className="text-gray-500 mb-6">{isOnline ? 'Waiting for requests...' : 'Go online to receive trips'}</p>
          <button 
            onClick={() => setIsOnline(!isOnline)}
            className={`w-full py-4 rounded-full font-bold text-lg text-white transition-colors ${
              isOnline ? 'bg-red-600 hover:bg-red-700' : 'bg-uber-blue hover:bg-blue-700'
            }`}
          >
            {isOnline ? 'GO OFFLINE' : 'GO ONLINE'}
          </button>
        </div>
      );
    }

    if (ride.status === 'REQUESTED') {
      return (
        <div className="space-y-4">
          <div className="text-center py-2">
            <h2 className="text-lg font-bold text-gray-500 uppercase tracking-widest">New Trip</h2>
            <div className="text-4xl font-bold mt-2">3 min</div>
            <p className="text-gray-600 text-sm">away</p>
          </div>
          <div className="bg-uber-gray p-4 rounded-xl space-y-2">
            <div className="font-bold">{ride.pickup}</div>
            <div className="text-gray-500 text-sm">Dropoff: {ride.destination}</div>
            <div className="text-xl font-bold text-green-600 mt-2">${ride.estimated_fare?.toFixed(2)}</div>
          </div>
          <div className="flex space-x-4 pt-2">
            <button 
              onClick={() => handleAction('/cancel', 'CANCELLED')}
              className="flex-1 bg-gray-200 text-uber-black py-4 rounded-full font-bold"
            >
              DECLINE
            </button>
            <button 
              onClick={() => handleAction('/accept', 'MATCHED')}
              className="flex-1 bg-uber-black text-uber-white py-4 rounded-full font-bold shadow-lg"
            >
              ACCEPT
            </button>
          </div>
        </div>
      );
    }

    if (ride.status === 'MATCHED') {
      return (
        <div className="space-y-4">
          <h2 className="text-xl font-bold">Pick up rider</h2>
          <div className="bg-uber-gray p-4 rounded-xl">
            <div className="font-bold">{ride.pickup}</div>
          </div>
          <button 
            onClick={() => handleAction('/start', 'ONGOING')}
            className="w-full bg-uber-blue text-uber-white py-4 rounded-full font-bold text-lg"
          >
            START TRIP
          </button>
        </div>
      );
    }

    if (ride.status === 'ONGOING') {
      return (
        <div className="space-y-4">
          <h2 className="text-xl font-bold">Drop off rider</h2>
          <div className="bg-uber-gray p-4 rounded-xl">
            <div className="font-bold">{ride.destination}</div>
          </div>
          <button 
            onClick={() => handleAction('/complete', 'COMPLETED')}
            className="w-full bg-red-600 text-uber-white py-4 rounded-full font-bold text-lg"
          >
            COMPLETE TRIP
          </button>
        </div>
      );
    }

    return null;
  };

  return (
    <div className="relative h-screen w-full bg-gray-100 overflow-hidden">
      <div className="absolute top-4 left-4 z-50 flex flex-col space-y-2">
        <div className="bg-white shadow px-4 py-2 rounded-full font-bold border-2 border-uber-blue">Uber Driver</div>
        <button onClick={handleLogout} className="bg-white shadow px-3 py-1 rounded-full text-xs text-red-500 font-bold">Logout</button>
      </div>

      <CustomMap 
        center={driverLocation} 
        driverLocation={driverLocation}
      />
      
      <BottomSheet>
        {renderContent()}
      </BottomSheet>
    </div>
  );
};

export default DriverView;

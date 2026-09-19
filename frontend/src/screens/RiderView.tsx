import React, { useState, useEffect } from 'react';
import CustomMap from '../components/Map';
import BottomSheet from '../components/BottomSheet';
import { useStore } from '../store/store';
import api from '../api/api';
import { MapPin, Navigation } from 'lucide-react';
import { wsClient } from '../ws/ws';

const defaultCenter: [number, number] = [37.7749, -122.4194]; // SF

const RiderView: React.FC = () => {
  const { ride, setRide, clearRide, setUser } = useStore();
  const [pickup, setPickup] = useState('');
  const [destination, setDestination] = useState('');
  const [loading, setLoading] = useState(false);

  const [mapCenter, setMapCenter] = useState<[number, number]>(defaultCenter);

  useEffect(() => {
    // Get actual user location
    if ('geolocation' in navigator) {
      navigator.geolocation.getCurrentPosition((position) => {
        setMapCenter([position.coords.latitude, position.coords.longitude]);
      });
    }

    const fetchActiveRide = async () => {
      try {
        const res = await api.get('/rides/active');
        if (res.data && res.data.ride_id) {
          setRide(res.data);
        }
      } catch (e) {
        // Ignore 404s or empty
      }
    };
    fetchActiveRide();
  }, [setRide]);

  // Hook into WS for real-time location and status updates
  useEffect(() => {
    const onMessage = (data: any) => {
      if (data.action === 'location_update' || data.event === 'location_update') {
        setRide({ driver_location: { lat: data.lat, lon: data.lon } });
      } else if (data.event === 'status_update') {
        setRide({ status: data.status });
      }
    };
    wsClient.on('location_update', onMessage);
    wsClient.on('status_update', onMessage);
    return () => {
      wsClient.off('location_update', onMessage);
      wsClient.off('status_update', onMessage);
    };
  }, [setRide]);

  const requestRide = async () => {
    if (!pickup || !destination) return;
    setLoading(true);
    try {
      const res = await api.post('/rides', {
        pickup,
        destination,
        estimated_fare: 15.50,
        estimated_time: 600,
        distance_km: 5.2
      });
      setRide({
        ride_id: res.data.ride_id,
        status: res.data.status,
        pickup,
        destination
      });
    } catch (e) {
      console.error(e);
      alert('Failed to request ride');
    } finally {
      setLoading(false);
    }
  };

  const handleLogout = () => {
    setUser(null, null);
  };

  const renderContent = () => {
    if (!ride.status || ride.status === 'CANCELLED') {
      return (
        <div className="space-y-4">
          <h2 className="text-xl font-bold">Where to?</h2>
          <div className="bg-uber-gray rounded-xl p-4 flex flex-col space-y-3 relative">
            <div className="flex items-center space-x-3">
              <div className="w-2 h-2 bg-uber-black rounded-full" />
              <input 
                value={pickup} onChange={(e) => setPickup(e.target.value)}
                placeholder="Current location" className="bg-transparent outline-none flex-1 text-sm font-medium" 
              />
            </div>
            <div className="absolute left-[22px] top-[30px] w-0.5 h-6 bg-gray-300" />
            <div className="h-px w-full bg-gray-300" />
            <div className="flex items-center space-x-3">
              <div className="w-2 h-2 bg-uber-black" />
              <input 
                value={destination} onChange={(e) => setDestination(e.target.value)}
                placeholder="Where to?" className="bg-transparent outline-none flex-1 text-sm font-medium" 
              />
            </div>
          </div>
          <button 
            onClick={requestRide} disabled={loading || !pickup || !destination}
            className="w-full bg-uber-black text-uber-white py-4 rounded-xl font-semibold text-lg hover:bg-uber-darkGray disabled:opacity-50 transition-colors"
          >
            {loading ? 'Requesting...' : 'Request Ride'}
          </button>
        </div>
      );
    }

    if (ride.status === 'REQUESTED') {
      return (
        <div className="flex flex-col items-center justify-center py-6">
          <div className="w-12 h-12 border-4 border-uber-gray border-t-uber-black rounded-full animate-spin mb-4" />
          <h2 className="text-xl font-bold">Finding your driver</h2>
        </div>
      );
    }

    if (ride.status === 'MATCHED' || ride.status === 'DRIVER_ARRIVING' || ride.status === 'ONGOING') {
      return (
        <div className="space-y-4">
          <div className="flex justify-between items-center bg-uber-gray p-4 rounded-xl">
            <div>
              <h2 className="text-xl font-bold">{ride.status.replace('_', ' ')}</h2>
              <p className="text-sm text-gray-600">Your driver is on the way</p>
            </div>
            <div className="bg-uber-white w-12 h-12 rounded-full flex items-center justify-center shadow">
              <span className="font-bold">5.0★</span>
            </div>
          </div>
          <div className="bg-uber-gray rounded-xl p-4 space-y-2">
            <div className="flex items-center space-x-2 text-sm">
              <MapPin size={16} /> <span>{ride.pickup}</span>
            </div>
            <div className="flex items-center space-x-2 text-sm">
              <Navigation size={16} /> <span>{ride.destination}</span>
            </div>
          </div>
        </div>
      );
    }

    if (ride.status === 'COMPLETED') {
      return (
        <div className="space-y-4 text-center">
          <div className="w-16 h-16 bg-green-100 text-green-600 rounded-full flex items-center justify-center mx-auto mb-2">
            <svg className="w-8 h-8" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={3} d="M5 13l4 4L19 7" /></svg>
          </div>
          <h2 className="text-2xl font-bold">You've arrived!</h2>
          <p className="text-gray-600">Hope you enjoyed your ride.</p>
          <div className="flex justify-center space-x-2 py-4 text-uber-yellow">
            {[1,2,3,4,5].map(i => <span key={i} className="text-3xl cursor-pointer">★</span>)}
          </div>
          <button 
            onClick={clearRide}
            className="w-full bg-uber-black text-uber-white py-4 rounded-xl font-semibold text-lg"
          >
            Done
          </button>
        </div>
      );
    }

    return null;
  };

  return (
    <div className="relative h-screen w-full bg-gray-100 overflow-hidden">
      <div className="absolute top-4 left-4 z-50 flex flex-col space-y-2">
        <div className="bg-white shadow px-4 py-2 rounded-full font-bold">Uber</div>
        <button onClick={handleLogout} className="bg-white shadow px-3 py-1 rounded-full text-xs text-red-500 font-bold">Logout</button>
      </div>

      <CustomMap 
        center={mapCenter} 
        driverLocation={ride.driver_location ? [ride.driver_location.lat, ride.driver_location.lon] : null}
      />
      
      <BottomSheet>
        {renderContent()}
      </BottomSheet>
    </div>
  );
};

export default RiderView;

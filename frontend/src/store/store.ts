import { create } from 'zustand';

interface User {
  user_id: string;
  name: string;
  email: string;
  role: 'PASSENGER' | 'DRIVER';
}

interface RideState {
  ride_id: string | null;
  status: 'REQUESTED' | 'MATCHED' | 'DRIVER_ARRIVING' | 'ONGOING' | 'COMPLETED' | 'CANCELLED' | null;
  pickup: string | null;
  destination: string | null;
  driver_location: { lat: number; lon: number } | null;
  estimated_fare: number | null;
}

interface AppState {
  user: User | null;
  token: string | null;
  ride: RideState;
  setUser: (user: User | null, token: string | null) => void;
  setRide: (ride: Partial<RideState>) => void;
  clearRide: () => void;
}

export const useStore = create<AppState>((set) => ({
  user: null,
  token: localStorage.getItem('token'),
  ride: {
    ride_id: null,
    status: null,
    pickup: null,
    destination: null,
    driver_location: null,
    estimated_fare: null,
  },
  setUser: (user, token) => {
    if (token) {
      localStorage.setItem('token', token);
    } else {
      localStorage.removeItem('token');
    }
    set({ user, token });
  },
  setRide: (rideUpdate) =>
    set((state) => ({
      ride: { ...state.ride, ...rideUpdate },
    })),
  clearRide: () =>
    set({
      ride: {
        ride_id: null,
        status: null,
        pickup: null,
        destination: null,
        driver_location: null,
        estimated_fare: null,
      },
    }),
}));

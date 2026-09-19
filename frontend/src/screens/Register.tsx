import React, { useState } from 'react';
import { useNavigate, Link } from 'react-router-dom';
import api from '../api/api';
import { useStore } from '../store/store';

const Register: React.FC = () => {
  const [name, setName] = useState('');
  const [email, setEmail] = useState('');
  const [phone, setPhone] = useState('');
  const [password, setPassword] = useState('');
  const [role, setRole] = useState<'PASSENGER' | 'DRIVER'>('PASSENGER');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  
  const setUser = useStore((state) => state.setUser);
  const navigate = useNavigate();

  const handleRegister = async (e: React.FormEvent) => {
    e.preventDefault();
    setLoading(true);
    setError('');
    
    try {
      const res = await api.post('/auth/register', { name, email, phone, password, role });
      // Depending on if register auto-logs in or not. For now we assume we need to login manually, or just mock setting user.
      if (res.data.token) {
        setUser(res.data.user, res.data.token);
        navigate('/');
      } else {
        navigate('/login');
      }
    } catch (err: any) {
      if (err.response && err.response.data && err.response.data.message) {
        setError(err.response.data.message);
      } else if (err.response && typeof err.response.data === 'string') {
        setError(err.response.data);
      } else {
        setError('Registration failed');
      }
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-uber-white flex flex-col justify-center px-6 py-12">
      <div className="w-full max-w-sm mx-auto">
        <h1 className="text-3xl font-bold mb-8">Create an account</h1>
        {error && <div className="text-red-500 mb-4 text-sm">{error}</div>}
        <form onSubmit={handleRegister} className="space-y-4">
          <input
            type="text"
            value={name}
            onChange={(e) => setName(e.target.value)}
            placeholder="Full Name"
            className="w-full px-4 py-3 bg-uber-gray rounded-lg focus:outline-none focus:ring-2 focus:ring-uber-black"
            required
          />
          <input
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            placeholder="name@example.com"
            className="w-full px-4 py-3 bg-uber-gray rounded-lg focus:outline-none focus:ring-2 focus:ring-uber-black"
            required
          />
          <input
            type="tel"
            value={phone}
            onChange={(e) => setPhone(e.target.value)}
            placeholder="Phone Number"
            className="w-full px-4 py-3 bg-uber-gray rounded-lg focus:outline-none focus:ring-2 focus:ring-uber-black"
            required
          />
          <input
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            placeholder="Password"
            className="w-full px-4 py-3 bg-uber-gray rounded-lg focus:outline-none focus:ring-2 focus:ring-uber-black"
            required
          />
          <select 
            value={role} 
            onChange={(e) => setRole(e.target.value as any)}
            className="w-full px-4 py-3 bg-uber-gray rounded-lg focus:outline-none focus:ring-2 focus:ring-uber-black"
          >
            <option value="PASSENGER">Rider</option>
            <option value="DRIVER">Driver</option>
          </select>
          <button
            type="submit"
            disabled={loading}
            className="w-full bg-uber-black text-uber-white font-semibold py-3 rounded-lg hover:bg-uber-darkGray transition-colors"
          >
            {loading ? 'Continuing...' : 'Continue'}
          </button>
        </form>
        <p className="mt-6 text-center text-sm text-gray-600">
          Already have an account? <Link to="/login" className="text-uber-blue underline">Log in</Link>
        </p>
      </div>
    </div>
  );
};

export default Register;

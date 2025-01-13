import { createClient } from '@vercel/edge-config';

export default async function handler(request, response) {
  const client = createClient(process.env.EDGE_CONFIG);
  
  if (request.method === 'POST') {
    // Increment the counter
    const currentCount = await client.get('thumbsCount') || 0;
    await client.set('thumbsCount', currentCount + 1);
    return response.status(200).json({ count: currentCount + 1 });
  } else if (request.method === 'GET') {
    // Get the current count
    const count = await client.get('thumbsCount') || 0;
    return response.status(200).json({ count });
  }
}
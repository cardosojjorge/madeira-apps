import type { LeadRequest } from "@/lib/types";

export interface LeadRecord extends LeadRequest {
  id: string;
  createdAt: string;
}

const leadsBuffer: LeadRecord[] = [];

export function addLead(payload: LeadRequest): LeadRecord {
  const record: LeadRecord = {
    ...payload,
    id: `lead-${Date.now()}-${Math.random().toString(16).slice(2, 7)}`,
    createdAt: new Date().toISOString(),
  };

  leadsBuffer.unshift(record);
  if (leadsBuffer.length > 1000) {
    leadsBuffer.pop();
  }

  return record;
}

export function listLeads(limit = 20): LeadRecord[] {
  return leadsBuffer.slice(0, limit);
}

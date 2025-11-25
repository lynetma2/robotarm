<script setup lang="ts">
import { ref } from 'vue'
import { ArrowLeft, Plus } from 'lucide-vue-next'
import { Button } from '@/components/ui/button'
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from '@/components/ui/card'

// Import Sub-components and Types
import WaypointList from './waypointList.vue'
import WaypointEditor from './waypointEditor.vue'
import type { Sequence } from './types'

// --- State ---
const selectedSequence = ref<Sequence | null>(null)

// Mock Data
const sequences = ref<Sequence[]>([
  {
    id: 1,
    title: 'Home Position',
    description: 'Reset arm to zero coordinates',
    waypoints: [{ id: 101, x: 0, y: 0, speed: 50 }]
  },
  {
    id: 2,
    title: 'Pickup Zone A',
    description: 'Move to conveyor belt input',
    waypoints: [{ id: 201, x: 50, y: 120, speed: 200 }]
  },
])

// --- Handlers ---

// Enter Edit Mode
const handleEdit = (seq: Sequence) => {
  selectedSequence.value = seq
}

// Exit Edit Mode
const handleBack = () => {
  selectedSequence.value = null
}

// Create New Sequence
const handleAdd = () => {
  const newSequence: Sequence = {
    id: Date.now(),
    title: 'New Sequence',
    description: 'Enter description here...',
    waypoints: []
  }
  sequences.value.push(newSequence)
  selectedSequence.value = newSequence // Auto-open the new sequence
}

// Delete Sequence (Triggered by Editor)
const handleDeleteSequence = (id: number) => {
  // Filter out the deleted sequence
  sequences.value = sequences.value.filter(s => s.id !== id)
  // Close the editor
  selectedSequence.value = null
}
</script>

<template>
  <Card class="w-[600px]">

    <CardHeader class="flex flex-row items-center space-y-0 gap-4">
      <Button
        v-if="selectedSequence"
        variant="outline"
        size="icon"
        @click="handleBack"
      >
        <ArrowLeft class="w-4 h-4" />
      </Button>

      <div class="flex flex-col">
        <CardTitle>
          {{ selectedSequence ? `Editing: ${selectedSequence.title}` : 'Waypoint Manager' }}
        </CardTitle>
        <CardDescription>
          {{ selectedSequence ? 'Drag to reorder. Expand to edit.' : 'Save and edit movements.' }}
        </CardDescription>
      </div>

      <div v-if="!selectedSequence" class="ml-auto">
        <Button variant="outline" size="icon" @click="handleAdd">
          <Plus class="w-4 h-4" />
        </Button>
      </div>
    </CardHeader>

    <CardContent>
      <div class="max-h-[400px] overflow-y-auto pr-4">

        <WaypointEditor
          v-if="selectedSequence"
          :sequence="selectedSequence"
          @delete-sequence="handleDeleteSequence"
        />

        <WaypointList
          v-else
          :sequences="sequences"
          @edit="handleEdit"
          @play="(id) => console.log('Playing', id)"
        />

      </div>
    </CardContent>

  </Card>
</template>

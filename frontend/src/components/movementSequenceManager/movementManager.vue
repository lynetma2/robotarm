<script setup lang="ts">
import { ref, computed }from 'vue'
import {
  Card,
  CardContent,
  CardDescription,
  CardFooter,
  CardHeader,
  CardTitle,
} from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
} from '@/components/ui/dropdown-menu'
import {
  Accordion,
  AccordionContent,
  AccordionItem,
  AccordionTrigger,
} from '@/components/ui/accordion'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from '@/components/ui/select'
import { Separator } from '@/components/ui/separator'
import { Switch } from '@/components/ui/switch'
import { ScrollArea } from '@/components/ui/scroll-area'

// Import icons from lucide-vue-next
import {
  Plus,
  MoreHorizontal,
  ChevronLeft,
  GripVertical,
  Trash2,
  Copy,
  Play,
  Pencil,
  Crosshair,
  MapPin,
} from 'lucide-vue-next'

// --- TYPE DEFINITIONS ---
interface Pose {
  x: number;
  y: number;
  z: number;
  roll: number;
  pitch: number;
  yaw: number;
}

interface Step {
  id: string;
  name: string;
  pose: Pose;
  interpolation: 'linear' | 'joint';
  speed: number;
}

interface Settings {
  loop: boolean;
}

// Represents a sequence that might be new (no ID)
interface Sequence {
  id: string | null;
  name: string;
  lastModified: string;
  steps: Step[];
  settings: Settings;
}

// Represents a sequence that is saved and has an ID
interface SavedSequence extends Sequence {
  id: string;
}


// --- MOCK DATA ---
// This is the initial data for your sequences.
const mockSequences: SavedSequence[] = [
  {
    id: 'seq_1',
    name: 'Pick & Place Routine',
    lastModified: '2h ago',
    steps: [
      {
        id: 'step_1a',
        name: 'Move to Home',
        pose: { x: 0, y: 150, z: 200, roll: 0, pitch: 90, yaw: 0 },
        interpolation: 'linear',
        speed: 100,
      },
      {
        id: 'step_1b',
        name: 'Approach Pick Target',
        pose: { x: 50, y: -100, z: 100, roll: 0, pitch: 90, yaw: 0 },
        interpolation: 'joint',
        speed: 80,
      },
    ],
    settings: {
      loop: false,
    },
  },
  {
    id: 'seq_2',
    name: 'Drawing Circle',
    lastModified: '1d ago',
    steps: [
      {
        id: 'step_2a',
        name: 'Start Point',
        pose: { x: 100, y: 100, z: 50, roll: 90, pitch: 0, yaw: 0 },
        interpolation: 'linear',
        speed: 50,
      },
    ],
    settings: {
      loop: true,
    },
  },
]

// --- COMPONENT STATE ---

// The main list of all sequences
const sequences = ref<SavedSequence[]>(mockSequences)

// Controls which view is active: 'list' or 'editor'
const view = ref<'list' | 'editor'>('list')

// Holds a deep copy of the sequence being edited.
// This is crucial so we can "discard" changes.
const editableSequence = ref<Sequence | null>(null)

// Stores a reference to the *original* sequence
// This helps find it in the main `sequences` array when saving.
const originalSequence = ref<SavedSequence | null>(null)

// --- COMPUTED PROPERTIES ---

// Dynamically sets the card title based on the current view
const cardTitle = computed<string>(() => {
  if (view.value === 'list') {
    return 'Movement Sequences'
  }
  if (editableSequence.value) {
    return editableSequence.value.id ? `Editing: ${editableSequence.value.name}` : 'New Sequence'
  }
  return 'Movement Sequences'
})

// --- FUNCTIONS ---

/**
 * Creates a deep copy of an object.
 * Using structuredClone for a modern, robust copy.
 */
function deepCopy<T>(obj: T): T {
  return structuredClone(obj)
}

/**
 * Generates a unique ID for new items.
 */
function generateId(prefix: string = 'item'): string {
  return `${prefix}_${Date.now()}_${Math.random().toString(36).substring(2, 9)}`
}

// --- View 1: List View Functions ---

/**
 * Called when the user clicks "+ New Sequence".
 * It creates a blank sequence object and switches to the editor view.
 */
function handleAddNewSequence() {
  originalSequence.value = null // It's a new sequence
  editableSequence.value = {
    id: null, // No ID yet, will be assigned on save
    name: 'New Sequence',
    lastModified: 'Just now',
    steps: [],
    settings: {
      loop: false,
    },
  }
  view.value = 'editor'
}

/**
 * Called when a user clicks on a sequence to edit it.
 * It creates a deep copy of the sequence for editing.
 */
function handleSelectSequence(sequence: SavedSequence) {
  originalSequence.value = sequence
  editableSequence.value = deepCopy(sequence)
  view.value = 'editor'
}

// --- Dropdown Menu Functions ---

function handleRun(sequenceId: string) {
  console.log('Running sequence:', sequenceId)
  // Add your robot "run" logic here
}

function handleDuplicate(sequenceId: string) {
  const seqToClone = sequences.value.find((s) => s.id === sequenceId)
  if (seqToClone) {
    const newSeq = {
      ...deepCopy(seqToClone),
      id: generateId('seq'),
      name: `${seqToClone.name} (Copy)`,
      lastModified: 'Just now',
    }
    sequences.value.push(newSeq)
  }
}

function handleDelete(sequenceId: string) {
  // We'll use a confirm in a real app, but for now, just delete.
  sequences.value = sequences.value.filter((s) => s.id !== sequenceId)
}

// --- View 2: Editor View Functions ---

/**
 * Called by the "Back" button in the editor.
 * Discards all changes and returns to the list view.
 */
function handleBackToList() {
  view.value = 'list'
  editableSequence.value = null
  originalSequence.value = null
}

/**
 * Saves the changes from `editableSequence` back to the main `sequences` list.
 */
function handleSaveChanges() {
  if (!editableSequence.value) return

  // Update last modified timestamp
  editableSequence.value.lastModified = 'Just now'

  if (originalSequence.value) {
    // This is an existing sequence, find and replace it
    const index = sequences.value.findIndex(
      (s) => s.id === originalSequence.value.id
    )
    if (index !== -1) {
      sequences.value[index] = deepCopy(editableSequence.value) as SavedSequence
    }
  } else {
    // This is a new sequence, add it to the list
    editableSequence.value.id = generateId('seq')
    sequences.value.push(deepCopy(editableSequence.value) as SavedSequence)
  }

  // Go back to the list
  handleBackToList()
}

/**
 * Discards changes by simply going back to the list
 * without saving. The `editableSequence` is just thrown away.
 */
function handleDiscardChanges() {
  handleBackToList()
}

/**
 * Adds a new, blank step to the sequence being edited.
 */
function handleAddNewStep() {
  if (editableSequence.value) {
    editableSequence.value.steps.push({
      id: generateId('step'),
      name: `New Step ${editableSequence.value.steps.length + 1}`,
      pose: { x: 0, y: 0, z: 0, roll: 0, pitch: 0, yaw: 0 },
      interpolation: 'linear',
      speed: 100,
    })
  }
}

/**
 * Deletes a step from the sequence being edited by its index.
 */
function handleDeleteStep(stepIndex: number) {
  if (editableSequence.value) {
    editableSequence.value.steps.splice(stepIndex, 1)
  }
}

// --- Step Action Stubs ---
function handleGoToPose(pose: Pose) {
  console.log('Moving robot to pose:', pose)
  // Add your robot "move" logic here
}

function handleCapturePose(step: Step) {
  console.log('Capturing current robot pose for step:', step.id)
  // This would fetch the robot's current 6-DOF position
  // and update the step's pose object.
  // For demo, let's just update to a random pose.
  step.pose = {
    x: +(Math.random() * 200 - 100).toFixed(1),
    y: +(Math.random() * 200 - 100).toFixed(1),
    z: +(Math.random() * 150).toFixed(1),
    roll: +(Math.random() * 180 - 90).toFixed(1),
    pitch: +(Math.random() * 180 - 90).toFixed(1),
    yaw: +(Math.random() * 180 - 90).toFixed(1),
  }
}

function handleRenameSequence() {
  const newName = prompt("Enter new sequence name:", editableSequence.value.name);
  if (newName) {
    editableSequence.value.name = newName;
  }
}
</script>

<template>
  <Card class="w-full max-w-lg mx-auto bg-white shadow-lg rounded-xl overflow-hidden">

    <!--
      VIEW 1: SEQUENCE LIST
      This view is shown when view.value === 'list'
    -->
    <template v-if="view === 'list'">
      <CardHeader>
        <div class="flex justify-between items-center">
          <CardTitle>{{ cardTitle }}</CardTitle>
          <Button @click="handleAddNewSequence">
            <Plus class="w-4 h-4 mr-2" />
            New Sequence
          </Button>
        </div>
      </CardHeader>
      <CardContent>
        <ScrollArea class="h-[450px] pr-3">
          <div class="space-y-4">
            <div
              v-for="sequence in sequences"
              :key="sequence.id"
              class="group"
            >
              <div class="flex items-center justify-between">
                <div
                  class="flex-1 cursor-pointer"
                  @click="handleSelectSequence(sequence)"
                >
                  <p class="font-semibold">{{ sequence.name }}</p>
                  <CardDescription>
                    {{ sequence.steps.length }} steps | Modified: {{ sequence.lastModified }}
                  </CardDescription>
                </div>

                <DropdownMenu>
                  <DropdownMenuTrigger as-child>
                    <Button variant="ghost" size="icon">
                      <MoreHorizontal class="w-4 h-4" />
                    </Button>
                  </DropdownMenuTrigger>
                  <DropdownMenuContent>
                    <DropdownMenuItem @click="handleRun(sequence.id)">
                      <Play class="w-4 h-4 mr-2" />
                      Run
                    </DropdownMenuItem>
                    <DropdownMenuItem @click="handleSelectSequence(sequence)">
                      <Pencil class="w-4 h-4 mr-2" />
                      Edit
                    </DropdownMenuItem>
                    <DropdownMenuItem @click="handleDuplicate(sequence.id)">
                      <Copy class="w-4 h-4 mr-2" />
                      Duplicate
                    </DropdownMenuItem>
                    <DropdownMenuSeparator />
                    <DropdownMenuItem @click="handleDelete(sequence.id)" class="text-red-600">
                      <Trash2 class="w-4 h-4 mr-2" />
                      Delete
                    </DropdownMenuItem>
                  </DropdownMenuContent>
                </DropdownMenu>
              </div>
              <Separator class="mt-4" />
            </div>

            <div v-if="sequences.length === 0" class="text-center text-gray-500 py-8">
              <p>No sequences found.</p>
              <p>Click "New Sequence" to start.</p>
            </div>
          </div>
        </ScrollArea>
      </CardContent>
    </template>

    <!--
      VIEW 2: SEQUENCE EDITOR
      This view is shown when view.value === 'editor'
    -->
    <template v-else-if="view === 'editor' && editableSequence">
      <CardHeader>
        <div class="flex items-center gap-2">
          <Button variant="outline" size="icon" @click="handleBackToList">
            <ChevronLeft class="w-4 h-4" />
          </Button>
          <CardTitle class="flex-1 whitespace-nowrap overflow-hidden text-ellipsis">
            {{ cardTitle }}
          </CardTitle>
          <Button variant="outline" size="sm" @click="handleRenameSequence">
            <Pencil class="w-3 h-3 mr-1.5" />
            Rename
          </Button>
        </div>
      </CardHeader>

      <CardContent>
        <Tabs default-value="steps">
          <TabsList class="grid w-full grid-cols-2">
            <TabsTrigger value="steps">Steps</TabsTrigger>
            <TabsTrigger value="settings">Settings</TabsTrigger>
          </TabsList>

          <!-- STEPS TAB -->
          <TabsContent value="steps">
            <ScrollArea class="h-[450px]">
              <Accordion type="single" collapsible class="w-full mt-4 pr-3">
                <AccordionItem
                  v-for="(step, index) in editableSequence.steps"
                  :key="step.id"
                  :value="step.id"
                >
                  <AccordionTrigger>
                    <div class="flex items-center w-full">
                      <GripVertical class="w-5 h-5 text-gray-400 mr-2 cursor-grab" />
                      <span class="font-medium">Step {{ index + 1 }}: {{ step.name }}</span>
                    </div>
                  </AccordionTrigger>
                  <AccordionContent class="space-y-6 pt-4">
                    <!-- Rename Step -->
                    <div class="space-y-2">
                      <Label :for="`stepName-${step.id}`">Step Name</Label>
                      <Input :id="`stepName-${step.id}`" v-model="step.name" />
                    </div>

                    <!-- 6-DOF Inputs -->
                    <div class="grid grid-cols-3 gap-x-4 gap-y-2">
                      <div class="space-y-1">
                        <Label :for="`stepX-${step.id}`">X</Label>
                        <Input :id="`stepX-${step.id}`" v-model.number="step.pose.x" type="number" />
                      </div>
                      <div class="space-y-1">
                        <Label :for="`stepY-${step.id}`">Y</Label>
                        <Input :id="`stepY-${step.id}`" v-model.number="step.pose.y" type="number" />
                      </div>
                      <div class="space-y-1">
                        <Label :for="`stepZ-${step.id}`">Z</Label>
                        <Input :id="`stepZ-${step.id}`" v-model.number="step.pose.z" type="number" />
                      </div>
                      <div class="space-y-1">
                        <Label :for="`stepRoll-${step.id}`">Roll</Label>
                        <Input :id="`stepRoll-${step.id}`" v-model.number="step.pose.roll" type="number" />
                      </div>
                      <div class="space-y-1">
                        <Label :for="`stepPitch-${step.id}`">Pitch</Label>
                        <Input :id="`stepPitch-${step.id}`" v-model.number="step.pose.pitch" type="number" />
                      </div>
                      <div class="space-y-1">
                        <Label :for="`stepYaw-${step.id}`">Yaw</Label>
                        <Input :id="`stepYaw-${step.id}`" v-model.number="step.pose.yaw" type="number" />
                      </div>
                    </div>

                    <!-- Interpolation & Speed -->
                    <div class="grid grid-cols-2 gap-4">
                      <div class="space-y-2">
                        <Label :for="`stepInterp-${step.id}`">Interpolation</Label>
                        <Select v-model="step.interpolation">
                          <SelectTrigger :id="`stepInterp-${step.id}`">
                            <SelectValue placeholder="Select type" />
                          </SelectTrigger>
                          <SelectContent>
                            <SelectItem value="linear">Linear</SelectItem>
                            <SelectItem value="joint">Joint</SelectItem>
                          </SelectContent>
                        </Select>
                      </div>
                      <div class="space-y-2">
                        <Label :for="`stepSpeed-${step.id}`">Speed (mm/s)</Label>
                        <Input :id="`stepSpeed-${step.id}`" v-model.number="step.speed" type="number" />
                      </div>
                    </div>

                    <!-- Step Actions -->
                    <div class="flex flex-wrap gap-2 justify-between pt-4">
                      <div class="flex flex-wrap gap-2">
                        <Button variant="outline" size="sm" @click="handleGoToPose(step.pose)">
                          <MapPin class="w-3 h-3 mr-1.5" />
                          Go To Pose
                        </Button>
                        <Button variant="outline" size="sm" @click="handleCapturePose(step)">
                          <Crosshair class="w-3 h-3 mr-1.5" />
                          Capture Pose
                        </Button>
                      </div>
                      <Button
                        variant="destructive"
                        outline
                        size="sm"
                        @click="handleDeleteStep(index)"
                      >
                        <Trash2 class="w-3 h-3 mr-1.5" />
                        Delete Step
                      </Button>
                    </div>

                  </AccordionContent>
                </AccordionItem>
              </Accordion>

              <Button
                variant="outline"
                class="w-full mt-6"
                @click="handleAddNewStep"
              >
                <Plus class="w-4 h-4 mr-2" />
                Add New Step
              </Button>
            </ScrollArea>
          </TabsContent>

          <!-- SETTINGS TAB -->
          <TabsContent value="settings" class="mt-6">
            <div class="space-y-4">
              <div class="flex items-center justify-between rounded-lg border p-3 shadow-sm">
                <div class="space-y-0.5">
                  <Label for="loop-sequence" class="text-base">Loop Sequence</Label>
                  <p class="text-sm text-muted-foreground">
                    Run this sequence continuously.
                  </p>
                </div>
                <Switch
                  id="loop-sequence"
                  v-model:checked="editableSequence.settings.loop"
                />
              </div>
            </div>
          </TabsContent>
        </Tabs>
      </CardContent>

      <CardFooter class="flex justify-end gap-3">
        <Button variant="ghost" @click="handleDiscardChanges">Discard</Button>
        <Button @click="handleSaveChanges">Save Changes</Button>
      </CardFooter>
    </template>
  </Card>
</template>

<style scoped>

</style>

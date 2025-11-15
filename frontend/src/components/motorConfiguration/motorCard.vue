<script setup lang="ts">
import { ref } from 'vue'
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from '@/components/ui/card'
import {
  Tabs,
  TabsContent,
  TabsList,
  TabsTrigger,
} from '@/components/ui/tabs'

// Import both of your child components
import MotorCurrent from '@/components/motorConfiguration/motorCurrent.vue'
import MotorStatus from '@/components/motorConfiguration/motorStatus.vue'

const id = ref(0)
</script>

<template>
  <Card class="w-[600px]">
    <CardHeader>
      <CardTitle>Motor {{ id }}</CardTitle>
      <CardDescription>
        View real-time status or change settings for motor with id = {{ id }}
      </CardDescription>
    </CardHeader>
    <CardContent>
      <Tabs default-value="status" class="w-full">
        <TabsList class="grid w-full grid-cols-2">
          <TabsTrigger value="status">
            📊 Real-time Status
          </TabsTrigger>
          <TabsTrigger value="settings">
            ⚙️ Settings
          </TabsTrigger>
        </TabsList>

        <TabsContent value="status">
          <MotorStatus :motor-id="id" />
        </TabsContent>

        <TabsContent value="settings">
          <MotorCurrent />
        </TabsContent>
      </Tabs>
    </CardContent>
  </Card>
</template>

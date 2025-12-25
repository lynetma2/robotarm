import { createRouter, createWebHistory } from 'vue-router'
import MotorConfigPage from "@/pages/motorConfigPage.vue";

// Lazy load pages
const DashboardPage = () => import('@/pages/dashboard/DashboardPage.vue')
const EditorPage = () => import('@/pages/editor/EditorPage.vue')

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      name: 'Dashboard',
      component: DashboardPage
    },
    {
      path: '/editor',
      name: 'Editor',
      component: EditorPage
    },
    {
      path: '/motorConfig',
      name: 'MotorConfig',
      component: MotorConfigPage
    }
  ]
})

export default router
